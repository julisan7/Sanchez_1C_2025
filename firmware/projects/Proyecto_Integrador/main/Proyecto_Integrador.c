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
 * |    Servo          |   ESP32   	|
 * |:-----------------:|:-----------|
 * | PWM (naranja)	   | 	CH0  	|
 * | GND (marron)	   | 	GND 	|
 * | VCC (rojo)	 	   | 	5 V 	|
 *
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 23/05/2025 | Creacion del documento                         |
 * | 27/05/2025 | Se definen datos internos y se esquematiza     |
 * |            | la tarea medir                                 |
 * | 28/05/2025 | Se esquematizan las tareas ventanas y luces    |
 * | 30/05/2025 | Implementacion del motor servo, funciona       |
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

#include "gpio_mcu.h"	//de los gpio para el servo y otras cositas
#include "servo_sg90.h" //driver del servo

#include "timer_mcu.h" //despues lo tengo q borrar

/*==================[macros and definitions]=================================*/

/*==================[internal data definition]===============================*/
bool ventanas_abiertas = 1;	  // false si las ventanas estan cerradas, true si estan abiertas
bool luces_prendidas = 0; // false si estan apagadas, true si estan prendidas
uint16_t medicion;			  // Variable para almacenar la medicion
uint16_t iluminacion_ideal;	  // Valor optimo con el cual se compara la medicion
uint8_t angulo_servo;		  // angulo que se mueve el servo

/*----------ESTO ES PARA PRUEBAS Y DESPUES SE BORRA----------*/
// #define CONFIG_TAREAS_PERIOD 1000*1000*60 // Periodo de las tareas en segundos porque el timer esta en microsegundos

TaskHandle_t medir_task_handle = NULL;		// tarea que mide
TaskHandle_t ventanas_task_handle = NULL;	// tarea que abre y cierra las ventanas
TaskHandle_t luces_task_handle = NULL;		// tarea que prende y apaga las luces
TaskHandle_t LeerEnviar_task_handle = NULL; // tarea que lee y envia datos de la uart ¿?

/*==================[internal functions declaration]=========================*/

/*----------ESTO ES PARA PRUEBAS Y DESPUES SE BORRA----------*/
/**
 * @brief Función invocada en la interrupción del timer tareas
 */
/*void FuncTimerTareas(void *param)
{
	vTaskNotifyGiveFromISR(ventanas_task_handle, pdFALSE); //Envía una notificación a la tarea asociada a ventanas
}
*/
//---------------------no se si deberia ir una de estas aca---------------------------
/**
 * @brief Función invocada en la interrupción del timer tareas
*
void FuncTimerTareas(void* param){
	vTaskNotifyGiveFromISR(medir_task_handle, pdFALSE);    // Envía una notificación a la tarea asociada a medir
	vTaskNotifyGiveFromISR(mostrar_task_handle, pdFALSE);    // Envía una notificación a la tarea asociada a mostrar
}
*/

/**
* @brief Funcion invocada para leer y enviar los datos
*
static void TareaLeerEnviar(void *pvParameter){ //convierte un dato de analogico a digital y depues envia el dato por la uart
	while (true){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		AnalogInputReadSingle(CH1, &datosConvertidos); //ver en q formato entra la iluminacion para convertir el dato ¿?
		UartSendString(UART_PC, ">Luz:");
		UartSendString(UART_PC, (char *)UartItoa(datosConvertidos, 10));
		UartSendString(UART_PC, "\r\n");
	}

}
*/

/**
 * @brief Funcion que mide el valor de la resistencia que sensa la luz
 */
/*static void TareaMedir (void *pvParameter){
	while(true){
		//ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // creo q aca no es portMAX_DELAY si no q quiero q sea cada 5 minutos (ver cuanto es el tiempo ideal) // La tarea espera en este punto hasta recibir una notificación
		//medicion=la entrada analogica || aca le asigno el valor a medicion
		if(medicion<iluminacion_ideal){
			//comparo afuera y adentro y veo si abro ventanas o prendo luces
		}
		if(medicion>=iluminacion_ideal){
			//jaja q hago si ta todo ok? podria informar por pantalla el valor de la resistencia ?
		}
	}
}
*/
/**
 * @brief Funcion que se encarga de abrir y cerrar las ventanas
 */

static void TareaVentanas(void *pvParameter)
{
	while (true)
	{
		// ulTaskNotifyTake(pdTRUE, portMAX_DELAY); //espera a recibir la notificacion
		vTaskDelay(5 * 1000 / portTICK_PERIOD_MS); //se abre/cierra cada 5 segundos ES DE PRUEBA
		switch (ventanas_abiertas)
		{
		case 0:
			angulo_servo = 90;
			ServoMove(SERVO_0, angulo_servo);
			ventanas_abiertas = 1;
			break;
		case 1:
			angulo_servo = -90;
			ServoMove(SERVO_0, angulo_servo);
			ventanas_abiertas = 0;
			break;
		}
	}
}
/**
 * @brief Funcion que se encarga de prender y apagar las luces
 */

static void TareaLuces (void *pvParameter){//Paco me dijo q lo haga con un led
	while(true){
		//ulTaskNotifyTake(pdTRUE, portMAX_DELAY); //espera a recibir la notificacion
		vTaskDelay(5 * 1000 / portTICK_PERIOD_MS); //se prende/apaga cada 5 segundos ES DE PRUEBA
			switch (luces_prendidas){
			case 0:
			//prender luces e informar
			break;
			case 1:
			//apagar luces e informar
			break;
		}
	}
}


/*==================[external functions definition]==========================*/
void app_main(void)
{
	/*
		timer_config_t timer_tareas = {
			.timer = TIMER_A,
			.period = CONFIG_TAREAS_PERIOD,
			.func_p = FuncTimerTareas,
			.param_p = NULL
		};
	*/
	ServoInit(SERVO_0, GPIO_1); // inicializacion ser servo, como puedo conectar hasta 4 servos pongo el primero y uso la salida analogica q da pwm

	//	TimerInit(&timer_tareas); // Inicializa el timer
	/*si lo hago con una foto resistencia, el valor de resistecia sube en la oscuridad, por lo q disminuye la corriente
	deberia conectarla a la entrada analogica de la placa y ahi varia la corriente, tengo alguna forma de variar el voltaje?
	deberia medir con una iluminacion q me guste y en base a eso hacer los calculos?
	*/
	xTaskCreate(&TareaVentanas, "Abrir y cerrar", 1024, NULL, 5, &ventanas_task_handle); // tarea q se encarga de leer y enviar
	xTaskCreate(&TareaLuces, "Prender y apagar", 1024, NULL, 5, &luces_task_handle); // tarea q se encarga de leer y enviar

	//	TimerStart(timer_tareas.timer); // Inicializa el conteo del timer tareas
}
/*==================[end of file]============================================*/