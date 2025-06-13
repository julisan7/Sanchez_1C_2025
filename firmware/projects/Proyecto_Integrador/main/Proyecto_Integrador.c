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

#include "rtc_mcu.h"//para la hora actual

/*==================[macros and definitions]=================================*/

/*==================[internal data definition]===============================*/
static bool ventanas_abiertas = 0;	  // 1 para abrir, 0 para cerrar
static bool prender_luces = 0; // 1 para prender, 0 para apagar
static uint16_t luz_interior;
static uint16_t luz_exterior=0;

bool prender_sistema=0; 

/**
 * @def iluminacion_ideal
 * @brief Valor en mV con el cual se compara la iluminacion interior
 */
uint16_t iluminacion_ideal=1650;	  // Valor optimo con el cual se compara la medicion en mV

uint8_t angulo_servo;		  // angulo que se mueve el servo

bool iluminacion_optima;

#define CONFIG_TAREAS_PERIOD 3*1000*1000 //Periodo para las meidiciones

TaskHandle_t medir_task_handle = NULL;		// tarea que mide
TaskHandle_t ventanas_task_handle = NULL;	// tarea que abre y cierra las ventanas
TaskHandle_t notificar_task_handle = NULL; // tarea que lee y envia datos de la uart

//ver por que esto no anda ¿?

rtc_t actual={
	.year=2025,
	.month=06,
	.mday=23,
   	.wday=1,
    .hour=0,	
    .min=0,	   
    .sec=5,
};


rtc_t apagado={
	.year=2025,
	.month=06,
    .mday=23,
   	.wday=1,
    .hour=0,
    .min=0,
    .sec=5	
};

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
//---------------VER SI ESTO FUNCIONA EN LA UART-----------------------		
		UartSendString(UART_PC, "Luz en el interior: ");
		UartSendString(UART_PC, (char *)UartItoa(luz_interior, 10));
		UartSendString(UART_PC, "\r\n");

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
		
		switch (ventanas_abiertas)
		{
		case 0:
			angulo_servo = 90;
			ServoMove(SERVO_0, angulo_servo);
			break;
		case 1:
			angulo_servo = -90;
			ServoMove(SERVO_0, angulo_servo);
			break;
		}
	}
}

/**
 *  @fn TareaNotificarUART(void *pvParameter)
 * @brief Tarea que notifica por UART el estado del programa
 * @param [in]
 * @return
 */
static void TareaNotificarUART(void *pvParameter){ //UART
	while (true){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
//-----------------------ESTO ES LO QUE DEBERIA ANDAR---------------------------		
		//UartSendString(UART_PC, "Luz en el interior: "); 
		//UartSendString(UART_PC, (char *)UartItoa(luz_interior, 10)); // esto me tira 0
		UartSendString(UART_PC, "Estatus a las: ");
		//hora y minutos
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

static void inicio(){

	uint16_t bytes_de_lectura=2;

	UartSendString(UART_PC, "Hola. Por favor ingrese la hora actual (hh): \r\n");
	UartReadBuffer(UART_PC,&actual.hour, bytes_de_lectura);
	UartSendString(UART_PC, "\r\nPor favor ingrese los minutos (mm): ");
	UartReadBuffer(UART_PC, &actual.min, bytes_de_lectura);
	UartSendString(UART_PC, "\r\nPor favor ingrese la hora a la cual debe apagarse el sistema (hh): ");
	UartReadBuffer(UART_PC, &apagado.hour, bytes_de_lectura);
	UartSendString(UART_PC, "\r\nPor favor ingrese los minutos (mm): ");
	UartReadBuffer(UART_PC, &apagado.min, bytes_de_lectura);

	RtcConfig(&actual);

	//RtcRead(&actual); cuando actual == apagado, se apaga (como lo implemento?)
	uint8_t hora=apagado.hour-actual.hour;
	uint8_t minutos=apagado.min-actual.min;

	UartSendString(UART_PC, "El sistema se apagara en: ");
	UartSendString(UART_PC, (char *)UartItoa(hora, 10));
	UartSendString(UART_PC, " horas, ");
	UartSendString(UART_PC, (char *)UartItoa(minutos, 10));
	UartSendString(UART_PC, " minutos. \r\n");

	prender_sistema=1;// preguntar si esto puede ir en los while(true)

}

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

	GPIOInit(GPIO_9, GPIO_OUTPUT); 

	ServoInit(SERVO_0, GPIO_1);

	UartInit(&my_uart);

	TimerInit(&timer_tareas);

	inicio();

	xTaskCreate(&TareaMedir, "Medir iluminacion", 4096, NULL, 5, &medir_task_handle);
	xTaskCreate(&TareaVentanas, "Abrir y cerrar", 1024, NULL, 5, &ventanas_task_handle);
	xTaskCreate(&TareaNotificarUART, "Notificar por UART", 2048, NULL, 5, &notificar_task_handle);
	TimerStart(timer_tareas.timer);

}
/*==================[end of file]============================================*/