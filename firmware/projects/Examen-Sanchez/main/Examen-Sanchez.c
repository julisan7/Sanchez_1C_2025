/*! @mainpage Examen Parcial 09/06/2025
 *
 * @section genDesc General Description
 *
 * El sistema está compuesto por un sensor de humedad y temperatura DHT11 y un  sensor analógico de radiación.
 * Se informa el valor de la humedad, de la temperatura y si existe el riesgo de nevada se prende el led rojo.
 * Cada 5 segundos se muestrea la radiacion ambiente y se informa su valor y si esta elevada se avisa por uart y se prende el led amarillo
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
 * | 	DATA	 	| 	GPIO_9		|
 * | 	GND		 	| 	GND 		|
 * 
 * |  Sensor de radiacion   |   ESP32   	|
 * |:----------------------:|:--------------|
 * | Entrada analogica	 	| 	CH1  		|
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
/**
 * @def CONFIG_HUMEDADYTEMPERATURA_PERIOD
 * @brief Tiempo en segundos cada cual ser realiza medicion de humedad y temperatura
 */
#define CONFIG_HUMEDADYTEMPERATURA_PERIOD 1*1000*1000 
/**
 * @def CONFIG_RADIACION_PERIOD
 * @brief Tiempo en segundos cada cual ser realiza medicion de la radiacion
 */
#define CONFIG_RADIACION_PERIOD 5*1000*1000 

/**
 * @def nevada
 * @brief booleano que cambia a 1 cuando se dan las condiciones para la nevada
 */
bool nevada=0;
/**
 * @def radiacion_alta
 * @brief 1 si los niveles de radiacion superan los 40mR/h (1320 mV en la entrada analogica)
 */
bool radiacion_alta=0;
/**
 * @def encendido
 * @brief booleano que cambia a false con el switch 2 y a true con el switch 1. Apaga todo el programa
 */
bool encendido=true;

/**
 * @def humedad
 * @brief humedad sensada en porcentaje
 */
float humedad;
/**
 * @def temperatura
 * @brief temperatura sensada en grados
 */
float temperatura;

/**
 * @def radiacion
 * @brief valor de la radiacion medida
 */
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
	while (true){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY); /* La tarea espera en este punto hasta recibir una notificación */
		if(encendido){
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
}
/**
 * @fn static void TareaVerificarNevada(void *pvParametro)
 * @brief Funcion que mide la humedad y la temperatura para verificar si hay condiciones para que se de una nevada
 * @param
 * @return nada
 */
static void TareaVerificarNevada(void *pvParametro){
	while (true){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY); /* La tarea espera en este punto hasta recibir una notificación */
		if (encendido){
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

}

/**
 * @fn static void TareaHumedadyTemperaturaUART(void *pvParametro)
 * @brief Funcion que notifica por UART el estado de la humedad y de la temperatura cada 1 segundo
 * @return nada
 */
static void TareaHumedadyTemperaturaUART(void *pvParametro){
	while(true){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY); /* La tarea espera en este punto hasta recibir una notificación */
		if (encendido){
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
}
/**
 * @fn static void TareaRadiacionUART(void *pvParametro){
 * @brief Funcion que notifica por UART el estado de la radiacion cada 5 segundos
 * @return nada
 */
static void TareaRadiacionUART(void *pvParametro){
	while(true){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY); /* La tarea espera en este punto hasta recibir una notificación */
		if (encendido){
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
	timer_config_t timer_humytemp = {
	.timer = TIMER_A,
	.period = CONFIG_HUMEDADYTEMPERATURA_PERIOD,
	.func_p = FuncTimer1seg,
	.param_p = NULL
	};
	
	timer_config_t timer_radiacion = { 
	.timer = TIMER_B,
	.period = CONFIG_RADIACION_PERIOD,
	.func_p = FuncTimer5seg,
	.param_p = NULL
	};

	analog_input_config_t canal_1 ={
	.input = CH1,		
	.mode = ADC_SINGLE, 
	.func_p = NULL,		
	.param_p = NULL,	
	.sample_frec = 0	
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

	SwitchActivInt(SWITCH_1, &Switch1On, NULL);
	SwitchActivInt(SWITCH_2, &Switch2Off, NULL); 

	xTaskCreate(&TareaMedirRadiacion, "Medir radiacion", 1024, NULL, 5, &medir_radiacion_task_handle);
	xTaskCreate(&TareaVerificarNevada, "Verificar nevada", 1024, NULL, 5, &verificar_nevada_task_handle);
	xTaskCreate(&TareaHumedadyTemperaturaUART, "Notificar por UART Humedad y Temperatura", 1024, NULL, 5, &notificar_humytemp_task_handle);
	xTaskCreate(&TareaRadiacionUART, "Notificar por UART Radiacion", 1024, NULL, 5, &notificar_radiacion_task_handle);

	TimerStart(timer_humytemp.timer);
	TimerStart(timer_radiacion.timer);
}
/*==================[end of file]============================================*/