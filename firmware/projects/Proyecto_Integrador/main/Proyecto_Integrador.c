/*! @mainpage Proyecto_Integrador
 *
 * @section genDesc General Description
 *
 * Mi proyecto consiste en sensar el nivel de iluminación interior y la iluminación exterior para abrir las cortinas o
 * prender las luces según corresponda. El circuito tendrá conectividad a una computadora para fijar los horarios
 * en los que debe funcionar e informar horas de encendido y los umbrales detectados en ese momento.
 *
 * <a href="https://drive.google.com/...">Operation Example</a>
 *
 * @section hardConn Hardware Connection
 *
 * |    Servo          |   ESP32   	|
 * |:-----------------:|:-----------|
 * | PWM (naranja)	   |   GPIO 1  	|
 * | GND (marron)	   | 	GND 	|
 * | VCC (rojo)	 	   | 	5 V 	|
 * 
 * 
 * |    LDR            |   ESP32   	| 5v
 * |:-----------------:|:-----------| 1k
 * | Luz interior	   |   GPIO 2  	| Aca deberia ir el cable de la entrada pq mido caida de tensionLDR
 * | Luz exterior	   |   GPIO 3  	| LDR
 * | RED LED           |   GPIO 9  	| GND  (esto en ambos casos, dejo la proto armada en el casillero) 
 * 									   
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 23/05/2025 | Creacion del documento                         |
 * | 27/05/2025 | Se definen datos internos y se esquematiza     |
 * |            | la tarea medir                                 |
 * | 28/05/2025 | Se esquematizan las tareas ventanas y luces    |
 * | 30/05/2025 | Implementacion del motor servo, funciona       |
 * | 06/06/2025 | Creacion de la tarea medir, no funciona        |
 * | 10/06/2025 | Cambios en la tarea medir y en la tarea luces, |
 * | 			| ambas compilan y flashean de forma correcta, 	 |
 * | 			| pero con problemas de logica. Ver si añadiendo |
 * | 			| el servo esto se arregla (igual reveer medir)  |
 * | 13/06/2025 | Se implementa la funcion de inicio			 |
 *
 * @author Julieta Sanchez (julieta.sanchez@ingenieria.uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>

#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "gpio_mcu.h"	//de los gpio para el servo y otras cositas
#include "servo_sg90.h" //driver del servo
#include "analog_io_mcu.h" //para leer la tension de las luces
#include "uart_mcu.h" //uart

#include "timer_mcu.h" //despues lo tengo q borrar

#include"rtc.h"//para la hora actual

/*==================[macros and definitions]=================================*/

/*==================[internal data definition]===============================*/
bool ventanas_abiertas = 1;	  // 1 para abrir, 0 para cerrar
bool prender_luces = 0; // 1 para prender, 0 para apagar
uint16_t luz_interior=0;
uint16_t luz_exterior=0;

bool prender_sistema=0; 

uint8_t hora_actual;
uint8_t minutos_actual;
uint8_t hora_apagado;
uint8_t minutos_apagado;

/*	typedef struct {
   uint16_t year;	    
   uint8_t  month;      
   uint8_t  mday;	    
   uint8_t  wday;	   
   uint8_t  hour;	   
   uint8_t  min;	   
   uint8_t  sec;	   
} rtc_t;
*/

//rtc_t actual; //por que me tira error? ya no 
//rtc_t apagado;

/**
 * @def iluminacion_ideal
 * @brief Valor en mV con el cual se compara la iluminacion interior
 */
uint16_t iluminacion_ideal=1650;	  // Valor optimo con el cual se compara la medicion en mV

uint8_t angulo_servo;		  // angulo que se mueve el servo
bool iluminacion_optima;

#define CONFIG_TAREAS_PERIOD 3*1000*1000 //Periodo para las meidiciones
//#define CONFIG_AVISO_PERIOD 5*1000*1000 //Periodo para informar en segundos porque el timer esta en microsegundos

TaskHandle_t medir_task_handle = NULL;		// tarea que mide
TaskHandle_t ventanas_task_handle = NULL;	// tarea que abre y cierra las ventanas
TaskHandle_t luces_task_handle = NULL;		// tarea que prende y apaga las luces
TaskHandle_t notificar_task_handle = NULL; // tarea que lee y envia datos de la uart ¿?

/*==================[internal functions declaration]=========================*/


/**
 *  @fn FuncTimer3seg(void *param)
 * @brief Funcion del Timer A que llama a las tareas cada 3 segundos
 * @param [in]
 * @return
 */
void FuncTimer3seg(void *param)
{
	vTaskNotifyGiveFromISR(medir_task_handle, pdFALSE); //Envía una notificación a la tarea asociada a medir
	vTaskNotifyGiveFromISR(ventanas_task_handle, pdFALSE); //Envía una notificación a la tarea asociada a ventanas
	vTaskNotifyGiveFromISR(notificar_task_handle, pdFALSE); //Envía una notificación a la tarea asociada a UART
//	vTaskNotifyGiveFromISR(luces_task_handle, pdFALSE); //Envía una notificación a la tarea asociada a luces
}

/**
 * @fn static void TareaMedir (void *pvParameter)
 * @brief Funcion que mide el valor de la resistencia que sensa la luz
 * @param [in]
 * @return
 */
static void TareaMedir (void *pvParameter)
{
	while(true){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		AnalogInputReadSingle(CH2,&luz_interior);
		AnalogInputReadSingle(CH3,&luz_exterior);
		if (ventanas_abiertas==1){
			if(luz_interior<iluminacion_ideal){
				if(luz_exterior>iluminacion_ideal){
					prender_luces=1;
					GPIOOn(GPIO_9);
				}
				else {
					if(luz_exterior<iluminacion_ideal){
						ventanas_abiertas=0;
					}
				}
			}
			else{
				iluminacion_optima=1;
			}
		}
		else{
			if(ventanas_abiertas==0){
				if(luz_exterior>iluminacion_ideal){
					GPIOOff(GPIO_9);
					ventanas_abiertas=1;
				}
				if(luz_exterior<iluminacion_ideal) {
					prender_luces=1;
					GPIOOn(GPIO_9);
				}
			}
		}
	}
}

/**
 * @fn static void TareaVentanas(void *pvParameter)
 * @brief Funcion que se encarga de abrir y cerrar las ventanas
 * @param 
 * @return nada
 */

static void TareaVentanas(void *pvParameter)
{	
	while (true)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY); 
		//vTaskDelay(2 * 1000 / portTICK_PERIOD_MS); //se abre/cierra cada 5 segundos ES DE PRUEBA
		switch (ventanas_abiertas)
		{
		case 0:
			angulo_servo = 90;
			ServoMove(SERVO_0, angulo_servo);
			//ventanas_abiertas = 1;
			break;
		case 1:
			angulo_servo = -90;
			ServoMove(SERVO_0, angulo_servo);
			//ventanas_abiertas = 0;
			break;
		}
	}
}

/**
 * @fn static void TareaLuces (void *pvParameter)
 * @brief Funcion que se encarga de prender y apagar las luces
 * @param [in]
 * @return
 */
/*
static void TareaLuces (void *pvParameter){//Paco me dijo q lo haga con un led
	GPIOOff(GPIO_9);//la luz inicia apagada
	while(true){
		//ulTaskNotifyTake(pdTRUE, portMAX_DELAY); //espera a recibir la notificacion

			switch (prender_luces){
				case 0:

					GPIOOn(GPIO_9);
					prender_luces=1;

					printf("se prende luz\r\n");
				break;

				case 1:

					GPIOOff(GPIO_9);
					prender_luces=0;

					printf("se apaga luz\r\n");
				break;
		}
		vTaskDelay(1 * 1000 / portTICK_PERIOD_MS); //se prende/apaga cada 5 segundos ES DE PRUEBA
		printf("delay luces\r\n");
	}
}
*/
/**
 *  @fn TareaNotificarUART(void *pvParameter)
 * @brief Tarea que notifica por UART el estado del programa
 * @param [in]
 * @return
 */
static void TareaNotificarUART(void *pvParameter){ //UART
	while (true){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY); //La tarea espera en este punto hasta recibir una notificación
	
		UartSendString(UART_PC, "Luz en el interior: ");
		luz_interior=10;
		UartSendString(UART_PC, (char *)UartItoa(luz_interior, 10));
		if(iluminacion_optima==1){
			UartSendString(UART_PC, ", la iluminacion es correcta. \r\n");
		}
		else UartSendString(UART_PC, ", la iluminacion es incorrecta. \r\n");
		if(ventanas_abiertas==1){
			UartSendString(UART_PC, "Las ventanas estan abiertas. \r\n");
		}
		else UartSendString(UART_PC, "Las ventanas estan cerradas. \r\n");
		if(prender_luces==1){
			UartSendString(UART_PC, "Las luces estan encendidas. \r\n");
		}
		else UartSendString(UART_PC, "Las luces estan apagadas. \r\n");
	}
}
/**
 * @fn static void inicio(void *pvParameter)
 * @brief Funcion que determina las horas de encendido y apagado
 * @param [in]
 * @return
 */
/*
static void inicio(){
	uint16_t bytes_de_lectura=2;//2?

	//tambien le hago ingresar el mes y el año?
	UartSendString(UART_PC, "Hola. Por favor ingrese la hora actual (hh): ");
	UartReadBuffer(UART_PC,&actual->hour, bytes_de_lectura);
	UartSendString(UART_PC, "\r\nPor favor ingrese los minutos (mm): ");
	UartReadBuffer(UART_PC,&actual->min, bytes_de_lectura);
	UartSendString(UART_PC, "\r\nPor favor ingrese la hora a la cual debe apagarse el sistema (hh): ");
	UartReadBuffer(UART_PC,&apagado->hour, bytes_de_lectura);
	UartSendString(UART_PC, "\r\nPor favor ingrese los minutos (mm): ");
	UartReadBuffer(UART_PC,&apagado->min, bytes_de_lectura);
	//como uso el rtc_mcu?
	RtcConfig(&actual);
	//RtcRead(&actual); cuando actual == apagado, se apaga (como lo implemento?)
	UartSendString(UART_PC, "\r\nSiendo las: ");//por ahi esto mejor no ponerlo asi no me complico
	//hora ingresada
	UartSendString(UART_PC, "\r\nEl sistema se apagara en: ");
	//hacer la cuenta
	UartSendString(UART_PC, " horas. ");
	prender_sistema=1;// preguntar si esto puede ir en los while(true)

}*/
/*==================[external functions definition]==========================*/
void app_main(void)
{
	timer_config_t timer_tareas = {
	.timer = TIMER_A,
	.period = CONFIG_TAREAS_PERIOD,
	.func_p = FuncTimer3seg,
	.param_p = NULL
	};

	analog_input_config_t canal_2 ={
	.input = CH2,		//canal2
	.mode = ADC_SINGLE, //single porque solo va por interrupciones
	.func_p = NULL,		// solo para modo continuo
	.param_p = NULL,	// solo para modo continuo
	.sample_frec = 0	//no lo vamos a usar
	};

	analog_input_config_t canal_3 ={
	.input = CH3,		//canal3
	.mode = ADC_SINGLE, //single porque solo va por interrupciones
	.func_p = NULL,		// solo para modo continuo
	.param_p = NULL,	// solo para modo continuo
	.sample_frec = 0	//no lo vamos a usar
	};

	serial_config_t my_uart = {
		.port = UART_PC,
		.baud_rate = 9600,
		.func_p = NULL,
		.param_p = NULL
	};

	AnalogInputInit(&canal_2);
	AnalogInputInit(&canal_3);

	GPIOInit(GPIO_9, GPIO_OUTPUT); //salida para el led **ACA MARCA UN PIQUITO RJO :(

	ServoInit(SERVO_0, GPIO_1); // inicializacion ser servo, como puedo conectar hasta 4 servos pongo el primero y uso la salida analogica q da pwm

	UartInit(&my_uart);
	TimerInit(&timer_tareas); // Inicializa el timer A

	
//	inicio();

	xTaskCreate(&TareaMedir, "Medir iluminacion", 4096, NULL, 5, &medir_task_handle);
	xTaskCreate(&TareaVentanas, "Abrir y cerrar", 1024, NULL, 5, &ventanas_task_handle);
	xTaskCreate(&TareaNotificarUART, "Notificar por UART", 2048, NULL, 5, &notificar_task_handle);
//	xTaskCreate(&TareaLuces, "Prender y apagar", 4096, NULL, 5, &luces_task_handle); // tarea q se encarga de leer y enviar
	TimerStart(timer_tareas.timer);

}
/*==================[end of file]============================================*/