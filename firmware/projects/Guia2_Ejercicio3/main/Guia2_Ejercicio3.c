/*! @mainpage Guia 2
 *
 * @section Ejercicio 3 de la guia 2
 *
 * Cree un nuevo proyecto en el que modifique la actividad del punto 2 agregando ahora el puerto serie.
 * Envíe los datos de las mediciones para poder observarlos en un terminal en la PC, siguiendo el siguiente formato:
 * 3 dígitos ascii + 1 carácter espacio + dos caracteres para la unidad (cm) + cambio de línea “ \r\n”
 * Además debe ser posible controlar la EDU-ESP de la siguiente manera:
 * Con las teclas “O” y “H”, replicar la funcionalidad de las teclas 1 y 2 de la EDU-ESP.
 *
 *
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |  EDU-CIAA-NXP |
 * |:--------------:|:--------------|
 * | 	ECHO     	| 	GPIO_3		|
 * | 	TRIGGER  	| 	GPIO_2		|
 * | 	+5V     	| 	+5V     	|
 * | 	GND      	| 	GND 		|
 *
 * |   Display      |   EDU-CIAA	|
 * |:--------------:|:-------------:|
 * | 	Vcc 	    |	5V      	|
 * | 	BCD1		| 	GPIO_20		|
 * | 	BCD2	 	| 	GPIO_21		|
 * | 	BCD3	 	| 	GPIO_22		|
 * | 	BCD4	 	| 	GPIO_23		|
 * | 	SEL1	 	| 	GPIO_19		|
 * | 	SEL2	 	| 	GPIO_18		|
 * | 	SEL3	 	| 	GPIO_9		|
 * | 	Gnd 	    | 	GND     	|
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 09/05/2025 | Creacion del documento		                 |
 * |            | Funciona el uart 								 |
 * |            |(informa por monitor la medicion)               |
 * | 16/05/2025 |Se agregaron las funcionalidades de O y H       |
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
#include "timer_mcu.h"
#include "led.h"
#include "hc_sr04.h"
#include "switch.h"
#include "lcditse0803.h"
#include "timer_mcu.h"
#include "uart_mcu.h"
#include "ctype.h"

/*==================[macros and definitions]=================================*/
#define CONFIG_TAREAS_PERIOD 1000 * 1000 // Periodo de las tareas en milisegundos porque el timer esta en microsegundos

bool conmutar_medicion = true; // Variable para controlar la medicion
bool guardar_medicion = false; // Variable para guardar la medicion
uint16_t medicion;			   // Variable para almacenar la medicion
uint16_t medicion_anterior;	   // Variable para almacenar la medicion anterior

/*==================[internal data definition]===============================*/
TaskHandle_t medir_task_handle = NULL;
TaskHandle_t mostrar_task_handle = NULL;
TaskHandle_t uart_task_handle = NULL;

/*==================[internal functions declaration]=========================*/
/**
 * @brief Función invocada en la interrupción del timer tareas
 */
void FuncTimerTareas(void *param)
{
	vTaskNotifyGiveFromISR(medir_task_handle, pdFALSE);	  /* Envía una notificación a la tarea asociada a medir*/
	vTaskNotifyGiveFromISR(mostrar_task_handle, pdFALSE); /* Envía una notificación a la tarea asociada a mostrar */
	vTaskNotifyGiveFromISR(uart_task_handle, pdFALSE);
}
/**
 * @brief Función invocada para medir la distancia y prender o apagar los leds si es necesario
 */
static void TareaMedir(void *pvParameter)
{
	while (true)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY); /* La tarea espera en este punto hasta recibir una notificación */
		if (conmutar_medicion)
		{
			medicion = HcSr04ReadDistanceInCentimeters(); // Lee la distancia en cm

			if (medicion < 10)
			{ /*se apagan todos los leds*/
				LedOff(LED_1);
				LedOff(LED_2);
				LedOff(LED_3);
			}
			else
			{
				if (medicion < 20)
				{ /*se enciende el led 1*/
					LedOn(LED_1);
					LedOff(LED_2);
					LedOff(LED_3);
				}
				else
				{
					if (medicion < 30)
					{ /*se enciende el led 1 y 2*/
						LedOn(LED_1);
						LedOn(LED_2);
						LedOff(LED_3);
					}
					else
					{ /*se encienden todos los leds*/
						LedOn(LED_1);
						LedOn(LED_2);
						LedOn(LED_3);
					}
				}
			}
		}
	}
}

/**
 * @brief Funcion invocada para activar o prender la medicion
 */
void Switch1OnOfMedicion()
{
	conmutar_medicion = !conmutar_medicion; // Cambia el estado de la variable de medicion
}

/**
 * @brief Funcion invocada para mantener en el display el valor de la medicion
 */
void Switch2HoldMedicion()
{
	guardar_medicion = !guardar_medicion; // Cambia el estado de la variable de guardar medicion
	medicion_anterior = medicion;		  // Guarda la medicion actual
}
/**
 * @brief Funcion invocada para mostrar la medicion en el display
 */
static void TareaMostrar(void *pvParameter)
{
	while (true)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY); /* La tarea espera en este punto hasta recibir una notificación */
		if (conmutar_medicion)
		{
			if (!guardar_medicion)
			{								// si guardar medicion es negativo muestra la medicion actual
				LcdItsE0803Write(medicion); // Muestra la medicion en el display
			}
			else
			{
				LcdItsE0803Write(medicion_anterior); // si guardar medicion es postivio muestra la medicion anterior
			}
		}
		else //Si se apreta la O, se apaga la pantalla
		{
			LcdItsE0803Off();
		}
	}
}
 /**
  * @brief Tarea de la UART
  */
void UartTask(void *param)// Declaracion de la tarea de UART
{ 
	while (true)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // La tarea espera en este punto hasta recibir una notificación
		UartSendString(UART_PC, "Se midieron: ");
		UartSendString(UART_PC, (char *)UartItoa(medicion, 10));
		UartSendString(UART_PC, " cm \r\n");
	}
}

/**
 * @brief Lee la entrada del usuario por el puerto serie y activa o mantiene la medicion
 */
void LeerEntrada()
{
	uint8_t letra;
	UartReadByte(UART_PC, &letra); // Lee la letra ingresada por el usuario
	if (letra == 'O')
	{
		Switch1OnOfMedicion(); //prende o apaga el lcd
	}
	if (letra == 'H')
	{
		Switch2HoldMedicion(); // Mantiene la medicion
		UartSendString (UART_PC, "Se debe volver a ingresar H para continuar midiendo");
	}
}

/*==================[external functions definition]==========================*/
void app_main(void)
{
	timer_config_t timer_tareas = {
		.timer = TIMER_A,
		.period = CONFIG_TAREAS_PERIOD,
		.func_p = FuncTimerTareas,
		.param_p = NULL};

	serial_config_t my_uart = {
		.port = UART_PC,
		.baud_rate = 9600,
		.func_p = LeerEntrada, // funcion que lee las teclas (O,H)
		.param_p = NULL};

	LedsInit();					// Inicializa los leds
	HcSr04Init(GPIO_3, GPIO_2); // Inicializa el HC_SR04
	SwitchesInit();				// Inicializa los switches
	LcdItsE0803Init();			// Inicializa el display LCD
	TimerInit(&timer_tareas);	// Inicializa el timer
	UartInit(&my_uart);			// Inicializa el UART

	SwitchActivInt(SWITCH_1, &Switch1OnOfMedicion, NULL); // Activa la interrupcion del switch 1
	SwitchActivInt(SWITCH_2, &Switch2HoldMedicion, NULL); // Activa la interrupcion del switch 2

	xTaskCreate(&TareaMedir, "Medir", 512, NULL, 5, &medir_task_handle);
	xTaskCreate(&TareaMostrar, "Mostrar", 512, NULL, 5, &mostrar_task_handle); // Crea la tarea para mostrar la medicion en el display
	xTaskCreate(&UartTask, "Uart", 512, NULL, 5, &uart_task_handle);		   // Crea la tarea para enviar la medicion por UART

	TimerStart(timer_tareas.timer); // Inicializa el conteo del timer tareas
}
/*==================[end of file]============================================*/