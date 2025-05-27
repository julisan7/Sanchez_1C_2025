/*! @mainpage Proyecto_Integrador
 *
 * @section genDesc General Description
 *
 * Mi proyecto consiste en sensar el nivel de iluminación interior y la iluminación exterior para abrir las cortinas o
 * prender las luces según corresponda. El circuito tendrá conectividad a una computadora para fijar los horarios
 * en los que debe funcionar e informar horas de encendido y los umbrales detectados en ese momento.
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
 * | 23/05/2025 | Creacion del documento                         |
 * | 27/05/2025 | Se definen datos internos y se esquematiza     |
 * |            | la tarea medir                                 |
 *
 * @author Julieta Sanchez (julieta.sanchez@ingenieria.uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
/*==================[macros and definitions]=================================*/

/*==================[internal data definition]===============================*/
bool ventanas_abiertas = false; //false si las ventanas estan cerradas, true si estan abiertas
bool luces_prendidas = false; //false si estan apagadas, true si estan prendidas
uint16_t medicion;			   // Variable para almacenar la medicion
uint16_t iluminacion_ideal;	   // Valor optimo con el cual se compara la medicion

TaskHandle_t medir_task_handle = NULL; //tarea que mide
/*==================[internal functions declaration]=========================*/
static void TareaMedir (void *pvParameter){
	while(true){
		//ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // creo q aca no es portMAX_DELAY si no q quiero q sea cada 5 minutos (ver cuanto es el tiempo ideal) /* La tarea espera en este punto hasta recibir una notificación */
		//aca le asigno el valor a medicion
		if(medicion<iluminacion_ideal){
			//comparo afuera y adentro y veo si abro ventanas o prendo luces
		}
		if(medicion>=iluminacion_ideal){
			//jaja q hago si ta todo ok?
		}
	}
}
/*==================[external functions definition]==========================*/
void app_main(void){
	printf("Hello world!\n");
}
/*==================[end of file]============================================*/