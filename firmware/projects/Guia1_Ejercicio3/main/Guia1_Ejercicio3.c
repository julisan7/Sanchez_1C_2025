/*! @mainpage Guia 1
 *
 * @section Ejercicio 3 de la guia 1
 *
 * Una función recibe un puntero a una estructura LED e indica que hacer con el led (prender, apagar, togglear)
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
 * @author Julieta Sanchez (julieta.sanchez@ingenieria.uner.edu.ar)
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
	uint16_t periodo;   //indica el tiempo de cada ciclo el milisegundos
} my_leds; 
/*==================[internal data definition]===============================*/
/**
 * @brief Prende, apaga o togglea el led segun el modo seleccionado
 * @param leds puntero a la estructura leds
 */
void quehacer(struct leds*leds)
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
		for (int i = 0; i < (2*leds->n_ciclos); i++)
		{
			switch (leds->n_led){
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
			vTaskDelay(leds->periodo / portTICK_PERIOD_MS);
		}

	break;
	}
}
/*==================[internal functions declaration]=========================*/


/*==================[external functions definition]==========================*/
void app_main(void){

LedsInit();

struct leds ledcito1, ledcito2;

ledcito1.mode=OFF;
ledcito1.n_led=1;
ledcito1.n_ciclos=30;
ledcito1.periodo=100;

ledcito2.mode=TOGGLE;
ledcito2.n_led=2;
ledcito2.n_ciclos=10;
ledcito2.periodo=500;


quehacer(&ledcito1);
quehacer(&ledcito2);


}
/*==================[end of file]============================================*/