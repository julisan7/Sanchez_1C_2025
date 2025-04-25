/*! @mainpage Guia 2
 *
 * @section Ejercicio 1 de la guia 2
 *
 * Mide la distancia, prende los leds en funcion de la distancia, muestra el valor de la distancia en el display LCD
 * y usa los botones TEC1 y TEC2 para activar o detener la medicion y mantener el resultado respectivamente. La medicion
 * se refresca cada 1 segundo
 *
 * <a href="https://drive.google.com/...">Operation Example</a>
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
 * | 11/04/2025 | Creacion del documento y de la tarea medir     |
 * | 25/04/2025 | La tarea medir funciona, agregamos switches y  | 
 * |            | el display.                                    |
 * |            |El ejercicio 1 funciona entero correctamente.    |
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

/*==================[macros and definitions]=================================*/
#define HC_SR04_H
#define ON 0 /*no se si lo uso*/
#define OFF 1

#define delay_tareas 1000

bool conmutar_medicion = true; // Variable para controlar la medicion
bool guardar_medicion = false; // Variable para guardar la medicion
uint16_t medicion; // Variable para almacenar la medicion
uint16_t medicion_anterior; // Variable para almacenar la medicion anterior

/*==================[internal data definition]===============================*/
TaskHandle_t medir_task_handle = NULL;
TaskHandle_t DetectarSwitches_task_handle = NULL;
TaskHandle_t mostrar_task_handle = NULL;


/*==================[internal functions declaration]=========================*/
/*
 * @brief Función invocada para medir la distancia y prender o apagar los leds si es necesario
 */
static void TareaMedir (void *pvParameter){
	while(true){
		if (conmutar_medicion){
			medicion=HcSr04ReadDistanceInCentimeters(); // Lee la distancia en cm
			if(medicion<10){ /*se apagan todos los leds*/
				LedOff(LED_1);
				LedOff(LED_2);
				LedOff(LED_3);
			}
			else{
				if(medicion<20){ /*se enciende el led 1*/
					LedOn(LED_1);
					LedOff(LED_2);
					LedOff(LED_3);
				}
				else{
					if(medicion<30){ /*se enciende el led 1 y 2*/
						LedOn(LED_1);
						LedOn(LED_2);
						LedOff(LED_3);
					}
					else{ /*se encienden todos los leds*/
						LedOn(LED_1);
						LedOn(LED_2);
						LedOn(LED_3);
					}
				}
			}
		}
		vTaskDelay(delay_tareas / portTICK_PERIOD_MS); /*delay de 1 segundo*/
	}
}
/*
* @brief Funcion invocada para detectar el estado del switch y si es el 1 deja de medir y si es el 2 mantiene la medicion en el display
*/
static void TareaDetectarSwitches (void *pvParameter){
	while(true){
		int8_t estado_switch = SwitchesRead(); // Lee el estado de los switches
		if(estado_switch == SWITCH_1){ // Si se presiona el switch 1
			conmutar_medicion = !conmutar_medicion; // Cambia el estado de la variable de medicion
		}
		if(estado_switch == SWITCH_2){ // Si se presiona el switch 2
			guardar_medicion = !guardar_medicion; // Cambia el estado de la variable de guardar medicion
			medicion_anterior = medicion; // Guarda la medicion actual
		}
		vTaskDelay(delay_tareas / portTICK_PERIOD_MS); /*delay de 1 segundo*/
	}
}
/*
 * @brief Funcion invocada para mostrar la medicion en el display
 */
static void TareaMostrar (void *pvParameter){
	while(true){
		if(!guardar_medicion){ // si guardar medicion es negativo muestra la medicion actual
			LcdItsE0803Write(medicion); // Muestra la medicion en el display
		}
		else{
			LcdItsE0803Write(medicion_anterior); // si guardar medicion es postivio muestra la medicion anterior
		}
		vTaskDelay(delay_tareas / portTICK_PERIOD_MS);
	}
}


/*==================[external functions definition]==========================*/
void app_main(void){
	LedsInit(); // Inicializa los leds
	HcSr04Init(GPIO_3,GPIO_2); // Inicializa el HC_SR04
	SwitchesInit(); // Inicializa los switches
	LcdItsE0803Init(); // Inicializa el display LCD

	xTaskCreate(&TareaDetectarSwitches, "DetectarSwitches", 512, NULL, 5, &DetectarSwitches_task_handle);
	xTaskCreate(&TareaMedir, "Medir", 512, NULL, 5, &medir_task_handle);
	xTaskCreate(&TareaMostrar, "Mostrar", 512, NULL, 5, &mostrar_task_handle); // Crea la tarea para mostrar la medicion en el display
}
/*==================[end of file]============================================*/