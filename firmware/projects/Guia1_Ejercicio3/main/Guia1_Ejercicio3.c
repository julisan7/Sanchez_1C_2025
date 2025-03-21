/*! @mainpage Template
 *
 * @section genDesc General Description
 *
 * This section describes how the program works.
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
 * | 12/09/2023 | Document creation		                         |
 *
 * @author Albano Peñalva (albano.penalva@uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>

#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "led.h"
#define ON 0
#define OFF 1
#define TOGGLE 2
/*==================[macros and definitions]=================================*/
struct leds //Estructura LED
{
    uint8_t mode;       //ON, OFF, TOGGLE
	uint8_t n_led;      //indica el número de led a controlar
	uint8_t n_ciclos;   //indica la cantidad de ciclos de ncendido/apagado
	uint16_t periodo;   //indica el tiempo de cada ciclo
} my_leds; 
/*==================[internal data definition]===============================*/
void quehacer(struct leds*leds) //Prende, apaga y togglea el led
{ 
	switch (leds->mode)
	{
	case ON:
		switch (leds->n_led)
		{
			case 1:
				LedOn(LED_1);
			break;
		
			case 2:
				LedOn(LED_2);
			break;
				case 3:
			LedOn(LED_3);
			break;
		}
	break;
	
	case OFF:
		switch (leds->n_led)
		{
			case 1:
				LedOff(LED_1);
			break;
			case 2:
				LedOff(LED_2);
			break;
			case 3:
				LedOff(LED_3);
			break;
		}
	break;

	case TOGGLE:
	switch (leds->n_led)
	{
		case 1:
			LedToggle(LED_1);
		break;
		case 2:
			LedToggle(LED_2);
		break;
		case 3:
			LedToggle(LED_3);
		break;
	}

	break;
	}
}
/*==================[internal functions declaration]=========================*/


/*==================[external functions definition]==========================*/
void app_main(void){

LedsInit();

struct leds ledcito1;

ledcito1.mode=ON;
ledcito1.n_led=1;
ledcito1.n_ciclos=30;
ledcito1.periodo=100;

quehacer(&ledcito1);


}
/*==================[end of file]============================================*/