/*! @mainpage Guia 1
 *
 * @section Ejercicio 6 de la guia 1
 *
 * Una función recibe un dato de 32 bits, la cantidad de dígitos de salida y dos vectores de estructuras del tipo  gpioConf_t.
 * Uno  de estos vectores es igual al definido en el punto anterior
 * y el otro vector mapea los puertos con el dígito del LCD a donde mostrar un dato. 
 * La función muestra por display el valor que recibe.
 * Se reutilizan las funciones creadas en el punto 4 y 5.
 *
 * <a href="https://drive.google.com/...">Operation Example</a>
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	D1	      	| 	GPIO_20		|
 * | 	D2	 	    | 	GPIO_21		|
 * | 	D3	 	    | 	GPIO_22		|
 * | 	D4	 	    | 	GPIO_23		|
 * | 	SEL_1	 	| 	GPIO_19		|
 * | 	SEL_2	 	| 	GPIO_18		|
 * | 	SEL_3	 	| 	GPIO_9		|
 * | 	+5V	     	| 	+5V		    |
 * | 	GND	    	| 	GND		    |
 *
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                               |
 * |:----------:|:----------------------------------------------------------|
 * | 28/03/2025 | Document creation		                                    |
 * | 28/03/2025 | Creacion del codigo basado en los ejercicios anteriores	|
 * | 11/04/2025 | Creacion de la documentacion                           	|
 *
 * @author Julieta Sanchez (julieta.sanchez@ingenieria.uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include "gpio_mcu.h"

/*==================[internal data definition]===============================*/
typedef struct // esto queda asi no toco nada
{
	gpio_t pin; /*!< GPIO pin number */
	io_t dir;	/*!< GPIO direction '0' IN;  '1' OUT*/
} gpioConf_t;
/*==================[internal functions declaration]=========================*/
/**
 * @brief Convierte un numero a un arreglo BCD.
 */
int8_t convertToBcdArray(uint32_t data, uint8_t digits, uint8_t *bcd_number) // del ejercicio 4
{
	for (int i = 0; i < digits; i++)
	{
		bcd_number[i] = data % 10;
		data = data / 10;
	}

	return 0;
}
/** 
 * @brief Cambia el estado del GPIO segun el bit correspondiente en el BCD ingresado.
 */
int8_t cambiarEstadoGPIO(uint8_t digitobcd, gpioConf_t *vectorgpio)
{ // del ejercicio 5
	for (int i = 0; i < 4;)
	{
		if (digitobcd & (1 << i))
		{							   // verdadero si el bit i es 1
			GPIOOn(vectorgpio[i].pin); // encender el led
		}
		else
		{
			GPIOOff(vectorgpio[i].pin); // apagar el led
		}
		i++;
	}

	return 0;
}

int8_t mostrarEnDisplay(uint32_t dato, gpioConf_t *vectorgpio, gpioConf_t *vectorposicion, uint8_t digitos)
{

	uint8_t arregloBCD[digitos];

	convertToBcdArray(dato, digitos, arregloBCD);

	for (int i = 0; i < digitos; i++)
	{
		cambiarEstadoGPIO(arregloBCD[i], vectorgpio); // encender el led
		GPIOOn(vectorposicion[i].pin);				  // encender el led
		GPIOOff(vectorposicion[i].pin);				  // apagar el led
	}
	return 0;
}

/*==================[external functions definition]==========================*/
void app_main(void)
{

	gpioConf_t vectorGPIO[4] = {
		{GPIO_20, GPIO_OUTPUT},
		{GPIO_21, GPIO_OUTPUT},
		{GPIO_22, GPIO_OUTPUT},
		{GPIO_23, GPIO_OUTPUT}}; // Vector de 4 elementos

	GPIOInit(vectorGPIO[0].pin, vectorGPIO[0].dir);
	GPIOInit(vectorGPIO[1].pin, vectorGPIO[1].dir);
	GPIOInit(vectorGPIO[2].pin, vectorGPIO[2].dir);
	GPIOInit(vectorGPIO[3].pin, vectorGPIO[3].dir);

	gpioConf_t vectorposicion[3] = {
		{GPIO_9, GPIO_OUTPUT},
		{GPIO_18, GPIO_OUTPUT},
		{GPIO_19, GPIO_OUTPUT},
	}; // Vector de 3 elementos

	GPIOInit(vectorposicion[0].pin, vectorposicion[0].dir);
	GPIOInit(vectorposicion[1].pin, vectorposicion[1].dir);
	GPIOInit(vectorposicion[2].pin, vectorposicion[2].dir);

	mostrarEnDisplay(123, vectorGPIO, vectorposicion, 3);
}
/*==================[end of file]============================================*/