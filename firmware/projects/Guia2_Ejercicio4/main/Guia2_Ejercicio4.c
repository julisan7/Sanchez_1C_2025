/*! @mainpage Guia 2
 *
 * @section Ejercico 4
 *
 * Diseñar e implementar una aplicación, basada en el driver analog io mcu.y el driver de transmisión serie uart mcu.h,
 * que digitalice una señal analógica y la transmita a un graficador de puerto serie de la PC.
 * Se debe tomar la entrada CH1 del conversor AD y la transmisión se debe realizar por la UART conectada al puerto serie de la PC,
 * en un formato compatible con un graficador por puerto serie. 
 * Sugerencias:
 * - Disparar la conversión AD a través de una interrupción periódica de timer. -> Hacer un timer
 * - Utilice una frecuencia de muestreo de 500Hz.
 * - Obtener los datos en una variable que le permita almacenar todos los bits del conversor.
 * - Transmitir los datos por la UART en formato ASCII a una velocidad de transmisión suficiente para realizar
 * conversiones a la frecuencia requerida.
 *
 * <a href="https://drive.google.com/...">Operation Example</a>
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	PIN_X	 	| 	GPIO_X		|
 *
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 16/05/2025 | Document creation, funciona	                     |
 *
 * @author Julieta Sanchez (julieta.sanchez@ingenieria.uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "analog_io_mcu.h"
#include "uart_mcu.h"
#include "timer_mcu.h"


/*==================[macros and definitions]=================================*/


/*==================[internal data definition]===============================*/
#define CONFIG_INTERRUPCION_PERIOD 20 * 1000 //2mil micro=2 mili pq periodo de las tareas en milisegundos porque el timer esta en microsegundos
#define BUFFER_SIZE 231

TaskHandle_t LeerEnviar_task_handle = NULL;
TaskHandle_t GenerarECG_task_handle = NULL;

uint16_t datosConvertidos;//son 12 bits pero no hay de 12 asi que uso 16 pq 8 es poco

const char ecg[BUFFER_SIZE] = {
    76, 77, 78, 77, 79, 86, 81, 76, 84, 93, 85, 80,
    89, 95, 89, 85, 93, 98, 94, 88, 98, 105, 96, 91,
    99, 105, 101, 96, 102, 106, 101, 96, 100, 107, 101,
    94, 100, 104, 100, 91, 99, 103, 98, 91, 96, 105, 95,
    88, 95, 100, 94, 85, 93, 99, 92, 84, 91, 96, 87, 80,
    83, 92, 86, 78, 84, 89, 79, 73, 81, 83, 78, 70, 80, 82,
    79, 69, 80, 82, 81, 70, 75, 81, 77, 74, 79, 83, 82, 72,
    80, 87, 79, 76, 85, 95, 87, 81, 88, 93, 88, 84, 87, 94,
    86, 82, 85, 94, 85, 82, 85, 95, 86, 83, 92, 99, 91, 88,
    94, 98, 95, 90, 97, 105, 104, 94, 98, 114, 117, 124, 144,
    180, 210, 236, 253, 227, 171, 99, 49, 34, 29, 43, 69, 89,
    89, 90, 98, 107, 104, 98, 104, 110, 102, 98, 103, 111, 101,
    94, 103, 108, 102, 95, 97, 106, 100, 92, 101, 103, 100, 94, 98,
    103, 96, 90, 98, 103, 97, 90, 99, 104, 95, 90, 99, 104, 100, 93,
    100, 106, 101, 93, 101, 105, 103, 96, 105, 112, 105, 99, 103, 108,
    99, 96, 102, 106, 99, 90, 92, 100, 87, 80, 82, 88, 77, 69, 75, 79,
    74, 67, 71, 78, 72, 67, 73, 81, 77, 71, 75, 84, 79, 77, 77, 76, 76,
};

/*==================[internal functions declaration]=========================*/
/*
* @brief Funcion invocada para leer y enviar los datos
*/
static void TareaLeerEnviar(void *pvParameter){ //convierte un dato de analogico a digital y depues envia el dato por la uart
	while (true){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		AnalogInputReadSingle(CH1, &datosConvertidos);
		UartSendString(UART_PC, ">Tension:");
		UartSendString(UART_PC, (char *)UartItoa(datosConvertidos, 10));
		UartSendString(UART_PC, "\r\n");
	}

}
/*
* @brief Esta funcion pasa la señal del buffer a analogico
*/
static void TareaGenerarECG(void *pvParameter){
	ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
	int i=0;
	while(true){
		AnalogOutputWrite(ecg[i]);
		i++;
		if (i == BUFFER_SIZE){
			i=0;
		}
	}
}

void FuncTimerTareas(void *param){
	vTaskNotifyGiveFromISR(LeerEnviar_task_handle, pdFALSE);	  /* Envía una notificación a la tarea asociada a Leer y medir*/
	vTaskNotifyGiveFromISR(GenerarECG_task_handle, pdFALSE);
}

/*==================[external functions definition]==========================*/
void app_main(void){
timer_config_t timer_interrupciones = { //timer
	.timer = TIMER_A,
	.period = CONFIG_INTERRUPCION_PERIOD,
	.func_p = FuncTimerTareas,
	.param_p = NULL};

serial_config_t my_uart = {
	.port = UART_PC, //pq va por USB
	.baud_rate = 115200, //pq mandamos muchos datos de 8 bits
	.func_p = NULL, 
	.param_p = NULL};

analog_input_config_t canal_1 ={
	.input = CH1,		//canal1
	.mode = ADC_SINGLE, //single porque solo va por interrupciones
	.func_p = NULL,		// solo para modo continuo
	.param_p = NULL,	// solo para modo continuo
	.sample_frec = 0	//no lo vamos a usar
};

TimerInit(&timer_interrupciones);
UartInit(&my_uart);
AnalogInputInit(&canal_1);
AnalogOutputInit(); //como solo una canal es salida (CH0) no hace falta aclarar

xTaskCreate (&TareaLeerEnviar, "Leer y Enviar", 1024, NULL, 5, &LeerEnviar_task_handle); //tarea q se encarga de leer y enviar
xTaskCreate (&TareaGenerarECG, "ECG", 1024, NULL, 5, &GenerarECG_task_handle); //tarea q se encarga de leer y enviar

TimerStart(timer_interrupciones.timer); //este va abajo de todo siempre
}
/*==================[end of file]============================================*/