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
 * 
 *  <a href="https://drive.google.com/...">Operation Example</a>
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
#include "dht11.h"
#include "led.h"
#include "analog_io_mcu.h"
#include "uart_mcu.h"
#include "switch.h"

/*==================[macros and definitions]=================================*/

/*==================[internal data definition]===============================*/

/*==================[internal functions declaration]=========================*/

/*==================[external functions definition]==========================*/
void app_main(void){
	printf("Hello world!\n");
}
/*==================[end of file]============================================*/