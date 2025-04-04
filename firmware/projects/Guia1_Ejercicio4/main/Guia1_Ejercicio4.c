/*! @mainpage Guia 1
 *
 * @section Ejercicio 4 de la guia 1
 *
 * Una función recibe un dato de 32 bits, la cantidad de dígitos de salida
 * y un puntero a un arreglo donde se almacene los n dígitos.
 * La función convierte el dato recibido a BCD y
 * guarda cada uno de los dígitos de salida en el arreglo pasado como puntero.
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
/*==================[macros and definitions]=================================*/

/*==================[internal data definition]===============================*/

/*==================[internal functions declaration]=========================*/
int8_t  convertToBcdArray (uint32_t data, uint8_t digits, uint8_t * bcd_number)
{
	/*
	int unidad=0, decena=0, centena=0;
	unidad=(data%10); // 428/10= 42 => 428%10=8
	decena=(data/10)%10; // 428/10=42 y => 42%10 = 2 
	centena=(data/100)%10; // 428/100=4 => 4%10=4
	
	bcd_number[0]=unidad;
	bcd_number[1]=decena;
	bcd_number[2]=centena;
	*/

	for(int i=0;i<digits;i++){
		bcd_number[i]=data%10;
		data=data/10;
	}

	return 0;
}


/*==================[external functions definition]==========================*/
void app_main(void){

	uint32_t dato=7775;
	uint8_t digitos=4;
	uint8_t arregloBCD[digitos];

	convertToBcdArray(dato,digitos, arregloBCD);

	for(int i=digitos-1;i>-1;i--){
		printf("%d",arregloBCD[i]);
	}
	/*
	printf("%d",arregloBCD[0]);
	printf("%d",arregloBCD[1]);
	printf("%d",arregloBCD[2]);
	*/
}
/*==================[end of file]============================================*/