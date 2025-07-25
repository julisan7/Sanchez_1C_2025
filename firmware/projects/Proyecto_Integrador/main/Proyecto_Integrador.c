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
 * | PWM (naranja)	   |   GPIO 1  	|
 * | GND (marron)	   | 	GND 	|
 * | VCC (rojo)	 	   | 	5 V 	|
 *
 *
 * |    LDR            |   ESP32   	|
 * |:-----------------:|:-----------|
 * | Luz interior	   |   GPIO 2  	|
 * | Luz exterior	   |   GPIO 3  	|
 * | RED LED           |   GPIO 9  	|
 *
 *
 * Esquema de conectividad en la protoboard de las LDR:
 * 
 * |5 v|-----------|1 k|-------|------| LDR |-------|GND|
 * 							GPIO 2/3
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
 * | 06/06/2025 | Creacion de la tarea medir, no funciona        |
 * | 10/06/2025 | Cambios en la tarea medir y en la tarea luces, |
 * | 			| ambas compilan y flashean de forma correcta, 	 |
 * | 			| pero con problemas de logica. Ver si añadiendo |
 * | 			| el servo esto se arregla						 |
 * | 13/06/2025 | El programa funciona en general, hay que ver el|
 * |			| problema descripto en la tarea de la uart	 	 |
 * | 15/06/2025 | Se agrega la tarea final, consultar si la 	 |
 * |			| logica los los whiles esta bien (y si puedo 	 |
 * |			| hacer un vtaskdelay en finalizar en lugar de 	 |
 * |			| agregarle un timer)				 			 |
 * |17/06/2025	| El programa corre de manera correcta.			 |
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

#include "gpio_mcu.h"	   //de los gpio para el servo y otras cositas
#include "servo_sg90.h"	   //driver del servo
#include "analog_io_mcu.h" //para leer la tension de las luces
#include "uart_mcu.h"	   //uart

#include "timer_mcu.h" //despues lo tengo q borrar

#include "rtc_mcu.h" //para la hora actual

/*==================[macros and definitions]=================================*/

/*==================[internal data definition]===============================*/
/**
 *  @var Ventanas_avbiertas
 * @brief variable booleana que esta en 1 para las ventanas abiertas y en 0 para las ventanas cerradas
 */
static bool ventanas_abiertas = 0;

/**
 * @var prender_luces
 * @brief variable booleana que esta en 1 para las luces prendidas y en 0 para las luces apagadas
 */
static bool prender_luces = 0;

/**
 *  @var luz_interior
 * @brief variable que almacena la medicion de la luz interior
 */
static uint16_t luz_interior;
/**
 *  @var luz_exterior
 * @brief variable que almacena la medicion de la luz exterior
 */
static uint16_t luz_exterior;

/**
 * @var finalizar
 * @brief variable booleana que cambia a true cuando el horario se cumple y se debe finalizar el programa
 */
static bool finalizar = false;

/**
 * @var iluminacion_ideal
 * @brief Valor en mV con el cual se compara la iluminacion interior
 */
uint16_t iluminacion_ideal = 1650; // Valor optimo con el cual se compara la medicion en mV

/**
 *  @var ansulo_servo
 * @brief angulo que se mueve el servo
 */
uint8_t angulo_servo;

/**
 *  @var iluminacion optima
 * @brief variable booleana que es true si la iluminacion es optima y false en el caso contrario
 */
bool iluminacion_optima;

#define CONFIG_TAREAS_PERIOD 3 * 1000 * 1000 // Periodo para las mediciones

TaskHandle_t medir_task_handle = NULL;			   // tarea que mide
TaskHandle_t ventanas_task_handle = NULL;		   // tarea que abre y cierra las ventanas
TaskHandle_t notificar_task_handle = NULL;		   // tarea que lee y envia datos de la uart
TaskHandle_t verificar_horario_task_handle = NULL; // tarea que llama a la funcion final

/**
 *  @var actual
 * @brief variable que almacena la hora actual una vez que se la configura por UART
 */
static rtc_t actual = {
	.year = 2025,
	.month = 06,
	.mday = 23,
	.wday = 1,
	.hour = 0,
	.min = 0,
	.sec = 5,
};

/**
 *  @var apagado
 * @brief variable que almacena la hora a la que se debe apagar el sistema
 */
rtc_t apagado = {
	.year = 2025,
	.month = 06,
	.mday = 23,
	.wday = 1,
	.hour = 20,
	.min = 00,
	.sec = 5};

/*==================[internal functions declaration]=========================*/

/**
 * @fn FuncTimer3seg(void *param)
 * @brief Funcion del Timer A que llama a las tareas cada 3 segundos
 * @return nada
 */
void FuncTimer3seg(void *param)
{
	vTaskNotifyGiveFromISR(medir_task_handle, pdFALSE);		// Envía una notificación a la tarea asociada a medir
	vTaskNotifyGiveFromISR(ventanas_task_handle, pdFALSE);	// Envía una notificación a la tarea asociada a ventanas
	vTaskNotifyGiveFromISR(notificar_task_handle, pdFALSE); // Envía una notificación a la tarea asociada a UART
}

/**
 * @fn static void TareaMedir (void *pvParameter)
 * @brief Funcion que mide el valor de la resistencia que sensa la luz y prende las luces de ser necesario
 * @return nada
 */
static void TareaMedir(void *pvParameter)
{
	while (true)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		if (finalizar == false)
		{
			AnalogInputReadSingle(CH2, &luz_interior);

			UartSendString(UART_PC, "Luz en el interior: ");
			UartSendString(UART_PC, (char *)UartItoa(luz_interior, 10));
			UartSendString(UART_PC, "\r\n");

			AnalogInputReadSingle(CH3, &luz_exterior);
			if (ventanas_abiertas == 1)
			{
				if (luz_interior < iluminacion_ideal)
				{
					if (luz_exterior > iluminacion_ideal)
					{
						prender_luces = 1;
						GPIOOn(GPIO_9);
					}
					else
					{
						if (luz_exterior < iluminacion_ideal)
						{
							ventanas_abiertas = 0;
						}
					}
				}
				else
				{
					iluminacion_optima = 1;
				}
			}
			else
			{
				if (ventanas_abiertas == 0)
				{
					if (luz_exterior > iluminacion_ideal)
					{
						GPIOOff(GPIO_9);
						ventanas_abiertas = 1;
					}
					if (luz_exterior < iluminacion_ideal)
					{
						prender_luces = 1;
						GPIOOn(GPIO_9);
					}
				}
			}
		}
	}
}

/**
 * @fn static void TareaVentanas(void *pvParameter)
 * @brief Funcion que se encarga de abrir y cerrar las ventanas
 * @return nada
 */

static void TareaVentanas(void *pvParameter)
{
	while (true)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		if (finalizar == false)
		{
			switch (ventanas_abiertas)
			{
			case 0:
				angulo_servo = 90;
				ServoMove(SERVO_0, angulo_servo);
				break;
			case 1:
				angulo_servo = -90;
				ServoMove(SERVO_0, angulo_servo);
				break;
			}
		}
	}
}

/**
 *  @fn TareaNotificarUART(void *pvParameter)
 * @brief Tarea que notifica por UART el estado del programa
 * @return nada
 */
static void TareaNotificarUART(void *pvParameter)
{
	while (true)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		if (finalizar == false)
		{
			UartSendString(UART_PC, "Estatus a las: ");

			RtcRead(&actual);
			UartSendString(UART_PC, (char *)UartItoa(actual.hour, 10));
			UartSendString(UART_PC, " : ");
			UartSendString(UART_PC, (char *)UartItoa(actual.min, 10));
			if (ventanas_abiertas == 1)
			{
				UartSendString(UART_PC, " hs. Las ventanas estan abiertas. \r\n");
			}
			else
				UartSendString(UART_PC, " hs. Las ventanas estan cerradas. \r\n");
			if (prender_luces == 1)
			{
				UartSendString(UART_PC, "Las luces estan encendidas. \r\n");
			}
			else
				UartSendString(UART_PC, "Las luces estan apagadas. \r\n");
		}
	}
}
/**
 * @fn static void Modificarhorarios()
 * @brief Funcion de interrupcion de la UART  para modificar el horario actual del programa
 * @return nada
 */

static void Modificarhorarios()
{

	uint8_t letra;
	uint8_t auxiliar = 0;
	UartReadByte(UART_PC, &letra);
	if (letra == 'h')
	{
		UartReadByte(UART_PC, &auxiliar);
		auxiliar = (auxiliar - 48) * 10;
		actual.hour = auxiliar;
		UartReadByte(UART_PC, &auxiliar);
		auxiliar = (auxiliar - 48);
		actual.hour = actual.hour + auxiliar;
	}
	if (letra == 'm')
	{
		UartReadByte(UART_PC, &auxiliar);
		auxiliar = (auxiliar - 48) * 10;
		actual.min = auxiliar;
		UartReadByte(UART_PC, &auxiliar);
		auxiliar = (auxiliar - 48);
		actual.min = actual.min + auxiliar;
	}
	if (RtcConfig(&actual) == true)
	{
		UartSendString(UART_PC, "Horario actualizado. \r\n");
	}
	else
	{
		UartSendString(UART_PC, "Horario incorrecto, vuelva a intentarlo. \r\n");
	}
}

//----------------------------------------------------------------------------------------------------------------------------------
/**
 * @fn static void final(void *pvParameter)
 * @brief Tarea que finaliza el programa cuando se cumple el horario
 * @return nada
 */

static void final(void *pvParameter)
{
	while (1)
	{
		if (finalizar == false)
		{
			RtcRead(&actual);
			if (actual.hour == apagado.hour)
			{
				if (actual.min == apagado.min)
				{
					UartSendString(UART_PC, "Son las: \r\n");
					UartSendString(UART_PC, (char *)UartItoa(apagado.hour, 10));
					UartSendString(UART_PC, " : ");
					UartSendString(UART_PC, (char *)UartItoa(apagado.min, 10));
					UartSendString(UART_PC, " hs.\r\n");
					UartSendString(UART_PC, "Tiempo cumplido.\r\n");
					UartSendString(UART_PC, "{Apagando}.\r\n");
					finalizar = true;
				}
			}
		}
		vTaskDelay(60 * 1000 / portTICK_PERIOD_MS);
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------

/*==================[external functions definition]==========================*/
void app_main(void)
{
	timer_config_t timer_tareas = {
		.timer = TIMER_A,
		.period = CONFIG_TAREAS_PERIOD,
		.func_p = FuncTimer3seg,
		.param_p = NULL
	};

	analog_input_config_t canal_2 = {
		.input = CH2,		
		.mode = ADC_SINGLE, 
		.func_p = NULL,		
		.param_p = NULL,	
		.sample_frec = 0	
	};

	analog_input_config_t canal_3 = {
		.input = CH3,		
		.mode = ADC_SINGLE, 
		.func_p = NULL,		
		.param_p = NULL,	
		.sample_frec = 0
	};

	serial_config_t my_uart = {
		.port = UART_PC,
		.baud_rate = 9600,
		.func_p = Modificarhorarios,
		.param_p = NULL};

	AnalogInputInit(&canal_2);
	AnalogInputInit(&canal_3);

	GPIOInit(GPIO_9, GPIO_OUTPUT);

	ServoInit(SERVO_0, GPIO_1);

	UartInit(&my_uart);

	TimerInit(&timer_tareas);

	xTaskCreate(&TareaMedir, "Medir iluminacion", 4096, NULL, 5, &medir_task_handle);
	xTaskCreate(&TareaVentanas, "Abrir y cerrar", 1024, NULL, 5, &ventanas_task_handle);
	xTaskCreate(&TareaNotificarUART, "Notificar por UART", 2048, NULL, 5, &notificar_task_handle);
	xTaskCreate(&final, "Horario de cierre", 2048, NULL, 5, &verificar_horario_task_handle);


	TimerStart(timer_tareas.timer);

}
/*==================[end of file]============================================*/