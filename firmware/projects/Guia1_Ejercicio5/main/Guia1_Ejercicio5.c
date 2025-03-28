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
 * @author Julieta Sanchez (julieta.sanchez@ingenieria.uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include "gpio_mcu.h" //Esto no esta en el 4

/*==================[internal functions declaration]=========================*/

typedef struct //esto queda asi no toco nada
{
	gpio_t pin;			/*!< GPIO pin number */
	io_t dir;			/*!< GPIO direction '0' IN;  '1' OUT*/
} gpioConf_t;

int8_t cambiarEstadoGPIO(uint8_t digitobcd,gpioConf_t *vectorgpio){ //el digitobcd es de 4 bits
	for(int i=0;i<4;){
		if(digitobcd & (1<<i)){ //verdadero si el bit i es 1
			GPIOOn(vectorgpio[i].pin); //encender el led
		}else{
			GPIOOff(vectorgpio[i].pin); //apagar el led
		}
		i++;
	}
	
	return 0;
}

void app_main(void){
	
	gpioConf_t vectorGPIO[4]={
		{GPIO_20, GPIO_OUTPUT},
		{GPIO_21, GPIO_OUTPUT},
		{GPIO_22, GPIO_OUTPUT},
		{GPIO_23, GPIO_OUTPUT}
	}; //Vector de 4 elementos

	GPIOInit(vectorGPIO[0].pin, vectorGPIO[0].dir);
	GPIOInit(vectorGPIO[1].pin, vectorGPIO[1].dir);
	GPIOInit(vectorGPIO[2].pin, vectorGPIO[2].dir);
	GPIOInit(vectorGPIO[3].pin, vectorGPIO[3].dir);

	uint8_t numerobcd=9; //ejemplo de 4 bits, el 9 en binario es 1001

	cambiarEstadoGPIO(numerobcd,vectorGPIO); //llamo a la funcion que cambia el estado de los GPIOs
}
/*==================[end of file]============================================*/