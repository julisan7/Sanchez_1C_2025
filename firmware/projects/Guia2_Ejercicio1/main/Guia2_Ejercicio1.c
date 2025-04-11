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
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 11/04/2023 | Creacion del documento y de la tarea medir     |
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
/*==================[macros and definitions]=================================*/
#define ON 0 /*no se si lo uso*/
#define OFF 1

#define delay_tareas 1000

bool conmutar_medicion = false; // Variable para controlar la medicion
uint8_t medicion; // Variable para almacenar la medicion

/*==================[internal data definition]===============================*/
TaskHandle_t medir_task_handle = NULL;


/*==================[internal functions declaration]=========================*/
/**
 * @brief Función invocada para medir la distancia y prender o apagar los leds si es necesario
 */
static void TareaMedir (void *pvParameter){
	while(true){
		while (conmutar_medicion){
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

static void TareaMostrar (void *pvParameter);

/*==================[external functions definition]==========================*/
void app_main(void){
	LedsInit(); // Inicializa los leds
	xTaskCreate(&TareaMedir, "Medir", 512, NULL, 5, &medir_task_handle);
}
/*==================[end of file]============================================*/