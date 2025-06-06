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
 * | Lector analogico  |   GPIO 2  	| LDR
 * | Lector analogico  |   GPIO 3  	| Aca deberia ir el cable de la entrada pq mido caida de tension
 * | RED LED           |   GPIO 9  	| 1K2
 * 									  GND  (esto en ambos casos, dejo la proto armada en el casillero)  
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

/*==================[macros and definitions]=================================*/

/*==================[internal data definition]===============================*/
bool abrir_ventanas = 1;	  // 1 para abrir, 0 para cerrar
bool prender_luces = 0; // 1 para prender, 0 para apagar
uint16_t luz_interior;
uint16_t luz_exterior;
uint16_t iluminacion_ideal=1650;	  // Valor optimo con el cual se compara la medicion en mV
uint8_t angulo_servo;		  // angulo que se mueve el servo
bool iluminacion_optima;

#define CONFIG_MEDICION_PERIOD 3*1000*1000 //Periodo para las meidiciones
#define CONFIG_AVISO_PERIOD 5*1000*1000 //Periodo para informar en segundos porque el timer esta en microsegundos

TaskHandle_t medir_task_handle = NULL;		// tarea que mide
TaskHandle_t ventanas_task_handle = NULL;	// tarea que abre y cierra las ventanas
TaskHandle_t luces_task_handle = NULL;		// tarea que prende y apaga las luces
TaskHandle_t notificar_task_handle = NULL; // tarea que lee y envia datos de la uart ¿?

/*==================[internal functions declaration]=========================*/


/**
 * @brief Funcion del Timer A que llama a las tareas cada 3 segundos
 */
/*void FuncTimer3seg(void *param)
{
	vTaskNotifyGiveFromISR(medir_task_handle, pdFALSE); //Envía una notificación a la tarea asociada a medir
	vTaskNotifyGiveFromISR(ventanas_task_handle, pdFALSE); //Envía una notificación a la tarea asociada a ventanas
	vTaskNotifyGiveFromISR(luces_task_handle, pdFALSE); //Envía una notificación a la tarea asociada a luces
}
*/
/**
 * @brief Funcion del Timer B que llama a las tareas cada 5 segundos
 */
/*void FuncTimer5seg(){
	vTaskNotifyGiveFromISR(notificar_task_handle, pdFALSE); //Envía una notificación a la tarea asociada a notificar
}
*/
/**
 * @brief Funcion que mide el valor de la resistencia que sensa la luz
 */
static void TareaMedir (void *pvParameter){
	while(true){
		//ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // creo q aca no es portMAX_DELAY si no q quiero q sea cada 5 minutos (ver cuanto es el tiempo ideal) // La tarea espera en este punto hasta recibir una notificación
		vTaskDelay(4 * 1000 / portTICK_PERIOD_MS);
		AnalogInputReadSingle(CH2,&luz_interior);
		AnalogInputReadSingle(CH3,&luz_exterior);
		if(luz_interior<iluminacion_ideal){
			if (abrir_ventanas==1){
				prender_luces=1;
				printf("hay que prender las luces");
			}
			else{
				if(luz_exterior>=iluminacion_ideal){
					abrir_ventanas=1;
					printf("hay que abrir la ventana");
				}
				else{
				prender_luces=1;
				}
			}
			
		}
		else iluminacion_optima=1;
	}
}

/**
 * @brief Funcion que se encarga de abrir y cerrar las ventanas
 */

/*static void TareaVentanas(void *pvParameter)
{	
	while (true)
	{
		//ulTaskNotifyTake(pdTRUE, portMAX_DELAY); //espera a recibir la notificacion
		vTaskDelay(4 * 1000 / portTICK_PERIOD_MS); //se abre/cierra cada 5 segundos ES DE PRUEBA
		switch (abrir_ventanas)
		{
		case 0:
			angulo_servo = 90;
			ServoMove(SERVO_0, angulo_servo);
			abrir_ventanas = 1;
			break;
		case 1:
			angulo_servo = -90;
			ServoMove(SERVO_0, angulo_servo);
			abrir_ventanas = 0;
			break;
		}
	}
}*/
/**
 * @brief Funcion que se encarga de prender y apagar las luces
 */

static void TareaLuces (void *pvParameter){//Paco me dijo q lo haga con un led
	while(true){
		//ulTaskNotifyTake(pdTRUE, portMAX_DELAY); //espera a recibir la notificacion
		vTaskDelay(4 * 1000 / portTICK_PERIOD_MS); //se prende/apaga cada 5 segundos ES DE PRUEBA
			switch (prender_luces){
			case 0:
			GPIOOn(GPIO_9);
			prender_luces=1;
			printf("se prende luz");
			break;
			case 1:
			GPIOOff(GPIO_9);
			prender_luces=0;
			printf("se apaga luz");
			break;
		}
	}
}

/**
 * @brief Tarea que notifica por UART el estado cada 5 segundos
 */
/*
static void TareaNotificarUART(void *pvParameter){ //UART
	while (true){
		//ulTaskNotifyTake(pdTRUE, portMAX_DELAY); //La tarea espera en este punto hasta recibir una notificación
		vTaskDelay(5 * 1000 / portTICK_PERIOD_MS);
		UartSendString(UART_PC, "Luz en el interior: ");
		UartSendString(UART_PC, (char *)UartItoa(luz_interior, 10));
		if(iluminacion_optima==1){
			UartSendString(UART_PC, ", la iluminacion es correcta. \r\n");
		}
		else UartSendString(UART_PC, ", la iluminacion es incorrecta. \r\n");
		if(abrir_ventanas==1){
			UartSendString(UART_PC, "Las ventanas estan abiertas. \r\n");
		}
		else UartSendString(UART_PC, "Las ventanas estan cerradas. \r\n");
		if(prender_luces==1){
			UartSendString(UART_PC, "Las luces estan encendidas. \r\n");
		}
		else UartSendString(UART_PC, "Las luces estan apagadas. \r\n");

	}
}
*/

/*==================[external functions definition]==========================*/
void app_main(void)
{

/*	timer_config_t timer_mediciones = { //timer para medir
	.timer = TIMER_A,
	.period = CONFIG_MEDICION_PERIOD,
	.func_p = FuncTimer3seg,
	.param_p = NULL
	};
	
	timer_config_t timer_notificacion = { //timer para notificar
	.timer = TIMER_B,
	.period = CONFIG_AVISO_PERIOD,
	.func_p = FuncTimer5seg,
	.param_p = NULL
	};
*/
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

/*	serial_config_t my_uart = {
		.port = UART_PC,
		.baud_rate = 9600,
		.func_p = NULL,
		.param_p = NULL};
*/
	AnalogInputInit(&canal_2);
	AnalogInputInit(&canal_3);

	GPIOInit(GPIO_9, GPIO_OUTPUT); //salida para el led
	GPIOOff(GPIO_9);

//	ServoInit(SERVO_0, GPIO_1); // inicializacion ser servo, como puedo conectar hasta 4 servos pongo el primero y uso la salida analogica q da pwm

//	UartInit(&my_uart);
//	TimerInit(&timer_mediciones); // Inicializa el timer A
//	TimerInit(&timer_notificacion); // Inicializa el timer B

	xTaskCreate(&TareaMedir, "Medir iluminacion", 1024, NULL, 5, &medir_task_handle);
//	xTaskCreate(&TareaVentanas, "Abrir y cerrar", 1024, NULL, 5, &ventanas_task_handle); // tarea q se encarga de leer y enviar
	xTaskCreate(&TareaLuces, "Prender y apagar", 1024, NULL, 5, &luces_task_handle); // tarea q se encarga de leer y enviar
//	xTaskCreate(&TareaNotificarUART, "Notificar por UART", 2048, NULL, 5, &notificar_task_handle);

//	TimerStart(timer_mediciones.timer);
//	TimerStart(timer_notificacion.timer);
}
/*==================[end of file]============================================*/