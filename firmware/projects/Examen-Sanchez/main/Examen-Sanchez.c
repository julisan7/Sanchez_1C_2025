/*! @mainpage Examen Parcial 09/06/2025
 *
 * @section genDesc General Description
 *
 * El sistema está compuesto por un sensor de humedad y temperatura DHT11 y un  sensor analógico de radiación.
 * se debe detectar el riesgo de nevada, la cual se da si la húmedad se encuentra por encima del 85%  y la temperatura entre 0 y 2ºC.
 * Para esto se deben tomar muestras cada 1 segundo y se envían por UART con el siguiente formato:
 * “Temperatura: 10ºC - Húmedad: 70%”
 *
 * 
 * Si se da la condición de riesgo de nevada se debe indicar el estado encendiendo el led Rojo de la placa,
 * además del envío de un mensaje por la UART:
 * “Temperatura: 1ºC - Húmedad: 90% - RIESGO DE NEVADA”
 * Además se debe monitorizar la radiación ambiente, para ello se cuenta con un sensor analógico que da una salida
 * de 0V para 0mR/h y 3.3V para una salida de 100 mR/h. Se deben tomar muestras de radiación cada 5 segundos,
 * enviando el mensaje por UART:
 * “Radiación 30mR/h”
 * Si se sobrepasan los 40mR/h se debe informar del riesgo por Radiación, encendiendo el led Amarillo de la placa,
 * y enviando en el mensaje:
 * “Radiación 50mR/h - RADIACIÓN ELEVADA”
 * Si no hay riesgo de nevada ni radiación excesiva, se indicará con el led Verde esta situación.
 * El botón 1 se utilizará para encender el dispositivo, comenzando el muestreo de los sensores y el envío de información.
 * El botón 2 apaga el dispositivo, deteniendo el proceso de muestreo y apagando todas las notificaciones. 
 * 
 * Nevada: humedad > 85% y temeperatura 0 y 2-> muestras cada 1 segundo y se envia a la uart
 * si nieva led rojo + mensaje uart
 * 
 * radiacion: 0V para 0mR/h y 3.3V para una salida de 100 mR/h -> cada 5 segundos. radiacion > 40 (1,32 V) =>led amarillo + mensajes uart 
 * 
 *  <a href="https://drive.google.com/...">Operation Example</a>
 *
 * @section hardConn Hardware Connection
 *
 * 
 * |   	DHT11		|   ESP-EDU 	|
 * |:--------------:|:--------------|
 * | 	VCC     	|	3V3     	|
 * | 	DATA	 	| 	GPIO_#		|
 * | 	GND		 	| 	GND 		|
 * 
 * |  Sensor de radiacion   |   ESP32   	|
 * |:----------------------:|:--------------|
 * | Entrada analogica	 	| 	CH1  		|
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
 * | 12/09/2023 | Document creation		                         |
 *
 * @author Julieta Sanchez (julieta.sanchez@ingenieria.uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include "dht11.h" //sensor temp y hum
#include "led.h" //leds placa
#include "analog_io_mcu.h" //sensor analogico de radiacion
#include "uart_mcu.h" //uart
#include "switch.h"// switch
#include "freertos/FreeRTOS.h" //tareas
#include "freertos/task.h" //tareas
#include "timer_mcu.h" //timers
#include "gpio_mcu.h"

/*==================[macros and definitions]=================================*/

/*==================[internal data definition]===============================*/
#define CONFIG_HUMEDADYTEMPERATURA_PERIOD 1*1000*1000 //Periodo para las mediciones de humedad y temperatura timer (timer en microsegundos)
#define CONFIG_RADIACION_PERIOD 5*1000*1000 //Periodo para la medicion de la radiacion

bool nevada=0;
bool radiacion_alta=0;
bool encendido=true;

float humedad; //en porcentual
float temperatura; //en grados

uint16_t radiacion;

TaskHandle_t medir_radiacion_task_handle = NULL;
TaskHandle_t verificar_nevada_task_handle = NULL;
TaskHandle_t notificar_humytemp_task_handle = NULL;
TaskHandle_t notificar_radiacion_task_handle = NULL;
/*==================[internal functions declaration]=========================*/
/**
 * @fn static void TareaMedirRadiacion(void *pvParametro)
 * @brief Funcion que mide el nivel de radiacion
 * @param
 * @return nada
 */
static void TareaMedirRadiacion(void *pvParametro){
	while (encendido){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY); /* La tarea espera en este punto hasta recibir una notificación */
		AnalogInputReadSingle(CH1,&radiacion);
		if (radiacion > 1320){
			radiacion_alta=1;
			LedOn(LED_2);
			LedOff(LED_1);
		}
		else {
			radiacion_alta=0;
			LedOff(LED_2);
		}
		if (nevada==0){
			LedOn(LED_1);//preguntamos y dijeron que el 1 es el verde
		}
	}
}
/**
 * @fn static void TareaVerificarNevada(void *pvParametro)
 * @brief Funcion que mide la humedad y la temperatura para verificar si hay condiciones para que se de una nevada
 * @param
 * @return nada
 */
static void TareaVerificarNevada(void *pvParametro){
	while (encendido){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY); /* La tarea espera en este punto hasta recibir una notificación */
		dht11Read(&humedad,&temperatura);
		if(humedad>85){
			if(temperatura>0 && temperatura <2){
				nevada=1;
				LedOn(LED_3); //preguntamos y dijeron que el 3 es el rojo
				LedOff(LED_1);
			}
			else {
				LedOff(LED_3);
			}
		}
	}

}

/**
 * @fn static void TareaHumedadyTemperaturaUART(void *pvParametro)
 * @brief Funcion que notifica por UART el estado de la humedad y de la temperatura cada 1 segundo
 * @return nada
 */
static void TareaHumedadyTemperaturaUART(void *pvParametro){
	while(encendido){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY); /* La tarea espera en este punto hasta recibir una notificación */
		UartSendString(UART_PC, "Temperatura:");
		UartSendString(UART_PC, (char *)UartItoa(temperatura, 10));
		UartSendString(UART_PC, " - Humedad:");
		UartSendString(UART_PC, (char *)UartItoa(humedad, 10));
		UartSendString(UART_PC, "%");
		if (nevada==1){
			UartSendString(UART_PC, " - RIESGO DE NEVADA \r\n");
		}
		else {
			UartSendString(UART_PC, "\r\n");
		}
	}
}
/**
 * @fn static void TareaRadiacionUART(void *pvParametro){
 * @brief Funcion que notifica por UART el estado de la radiacion cada 5 segundos
 * @return nada
 */
static void TareaRadiacionUART(void *pvParametro){
	while(encendido){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY); /* La tarea espera en este punto hasta recibir una notificación */
		UartSendString(UART_PC, "Radiación:");
		UartSendString(UART_PC, (char *)UartItoa(radiacion, 10));
		UartSendString(UART_PC, "mR/h");
		if (radiacion_alta==1){
			UartSendString(UART_PC, " - RADIACÓN ELEVADA\r\n");
		}
		else {
			UartSendString(UART_PC, "\r\n");
		}
	}
}

/**
 * @fn FuncTimer5seg()
 * @brief Funcion que notifica a las tareas cada 5 segundos
 * @return nada
 */
void FuncTimer5seg(){
	vTaskNotifyGiveFromISR(medir_radiacion_task_handle, pdFALSE); /* Envía una notificación a la tarea asociada a notificar*/
	vTaskNotifyGiveFromISR(notificar_radiacion_task_handle, pdFALSE);
}
/**
 * @fn FuncTimer1seg()
 * @brief Funcion que notifica a las tareas cada 1 segundo
 * @return nada
 */
void FuncTimer1seg(){
	vTaskNotifyGiveFromISR(verificar_nevada_task_handle, pdFALSE);    /* Envía una notificación a la tarea asociada a medir agua*/
	vTaskNotifyGiveFromISR(notificar_humytemp_task_handle, pdFALSE);
}
/**
 * @fn Switch1On()
 * @brief Interrupccion del switch 1, prende el programa
 * @return nada
 */
void Switch1On()
{
	encendido = true;
}

/**
 * @fn void Switch2Off()
 * @brief Interrupccion del switch 2, apaga el programa
 * @return nada
 */
void Switch2Off()
{
	encendido=false;
}


/*==================[external functions definition]==========================*/
void app_main(void){
	timer_config_t timer_humytemp = { //timer para medir
	.timer = TIMER_A,
	.period = CONFIG_HUMEDADYTEMPERATURA_PERIOD,
	.func_p = FuncTimer1seg,
	.param_p = NULL
	};
	
	timer_config_t timer_radiacion = { //timer para notificar
	.timer = TIMER_B,
	.period = CONFIG_RADIACION_PERIOD,
	.func_p = FuncTimer5seg,
	.param_p = NULL
	};

	analog_input_config_t canal_1 ={
	.input = CH1,		//canal1
	.mode = ADC_SINGLE, //single porque solo va por interrupciones
	.func_p = NULL,		// solo para modo continuo
	.param_p = NULL,	// solo para modo continuo
	.sample_frec = 0	//no lo vamos a usar
	};

	serial_config_t my_uart = {
	.port = UART_PC,
	.baud_rate = 115200,
	.func_p = NULL, 
	.param_p = NULL};


	TimerInit(&timer_humytemp); // Inicializa el timer A
	TimerInit(&timer_radiacion); // Inicializa el timer B

	LedsInit();	

	UartInit(&my_uart);

	GPIOInit(GPIO_9,GPIO_INPUT);
	dht11Init(GPIO_9);

	AnalogInputInit(&canal_1);

	SwitchActivInt(SWITCH_1, &Switch1On, NULL); // Activa la interrupcion del switch 1
	SwitchActivInt(SWITCH_2, &Switch2Off, NULL); // Activa la interrupcion del switch 2


	xTaskCreate(&TareaMedirRadiacion, "Medir radiacion", 1024, NULL, 5, &medir_radiacion_task_handle);
	xTaskCreate(&TareaVerificarNevada, "Verificar nevada", 1024, NULL, 5, &verificar_nevada_task_handle);
	xTaskCreate(&TareaHumedadyTemperaturaUART, "Notificar por UART Humedad y Temperatura", 1024, NULL, 5, &notificar_humytemp_task_handle);
	xTaskCreate(&TareaRadiacionUART, "Notificar por UART Radiacion", 1024, NULL, 5, &notificar_radiacion_task_handle);

	TimerStart(timer_humytemp.timer);
	TimerStart(timer_radiacion.timer);
}
/*==================[end of file]============================================*/