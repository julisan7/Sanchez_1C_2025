/*! @mainpage Practica Preparcial
 *
 * @section Parcial 11-06-24
 *
 * Se pretende diseñar un dispositivo basado en la ESP-EDU que permita controlar el riego y el pH de una plantera.
 * El sistema está compuesto por una serie de recipientes con agua, una solución de pH ácida y otra básica, un sensor de húmedad y uno de pH,
 * y tres bombas peristálticas para los líquidos.
 * Por un lado se debe controlar el suministro de agua. El sensor de humedad se conecta a un GPIO de la placa y cambia su estado
 * de “0” a “1” lógico cuando la humedad presente en la tierra es inferior a la necesaria. La bomba de agua deberá encenderse en este caso.
 * El pH de la plantera se debe mantener entre el rango de 6 a 6,7. Se cuenta con un sensor de pH analógico que brinda una salida
 * de 0 a 3V para el rango de pH de 0 a 14. Por debajo de 6 se debe encender la bomba de la solución básica, por encima de 6.7
 * se debe encender la bomba de la solución ácida. La medición de agua y pH se debe realizar cada 3 segundos. Las bombas se
 * encienden colocando en alto un GPIO conectado a las mismas.
 * 
 * Se informará el estado del sistema a través de la UART mediante mensajes cada 5 segundos de la siguiente manera: “pH: 6.5, humedad correcta”
 * Además se deberá informar si alguna de las bombas está encendida: “pH: 6.9, humedad incorrecta” “Bomba de pHA encendida” “Bomba de agua encendida”
 * Por último, se deben utilizar las teclas 1 y 2 para iniciar y detener el sistema, respectivamente. Utilice al menos dos tareas.
 *
 * 
 * Humedad: GPIO y va de 0 a 1 cuando la humedad es menor a la necesaria (se prende la bomba en 1)
 * pH: CH1 analogico, de 0 a 3 v para 0 a 14. Por debajo de 6 se prende la bomba basica. Por arriba de 6.7 se prende la bomba acida (se prenden con un 1)
 * pH y agua se miden cada 3 segundos.
 * 
 * Las bombas se prenden con un alto
 * 
 * <a href="https://drive.google.com/...">Operation Example</a>
 *
 * @section hardConn Hardware Connection
 *
 * 
 * |     Peripheral      |   ESP32   	|
 * |:-------------------:|:-------------|
 * | Sensor pH analogico | 	GPIO_1 		| de 0 a 3 V para de 0 a 14 (6 = 1,286 V; 6,7 = 1,436 V)
 * | Sensor de humedad   | 	GPIO_9		|
 * |   	Bomba agua 		 |	GPIO_18		|
 * |   	Bomba acida		 | 	GPIO_19		|
 * |   	Bomba basica 	 | 	GPIO_20		|
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
#include "freertos/FreeRTOS.h" //tareas
#include "freertos/task.h" //tareas
#include "analog_io_mcu.h" //entrada analogica para el ph
#include "gpio_mcu.h" //GPIOs
#include "uart_mcu.h" //uart
#include "timer_mcu.h" //timers
#include "switch.h" //switches de la placa

/*==================[macros and definitions]=================================*/


/*==================[internal data definition]===============================*/
#define CONFIG_MEDICION_PERIOD 3*1000*1000 //Periodo para las meidiciones de agua y ph en segundos porque el timer esta en microsegundos
#define CONFIG_AVISO_PERIOD 5*1000*1000 //Periodo para informar en segundos porque el timer esta en microsegundos

bool bomba_agua=0; //0 apagada, 1 prendida
bool bomba_acida=0; //0 apagada, 1 prendida
bool bomba_basica=0; //0 apagada, 1 prendida
bool encendido=true;

uint16_t valor_ph=0; //de 0 a 3 V
bool humedad=0; //1 baja, 0 bien

TaskHandle_t medir_agua_task_handle = NULL;		// tarea que mide
TaskHandle_t medir_ph_task_handle = NULL;
TaskHandle_t notificar_task_handle = NULL;

/*==================[internal functions declaration]=========================*/
/**
 * @brief Funcion del Timer A que llama a las tareas cada 3 segundos
 */
void FuncTimer3seg(){
	vTaskNotifyGiveFromISR(medir_agua_task_handle, pdFALSE);    /* Envía una notificación a la tarea asociada a medir agua*/
	vTaskNotifyGiveFromISR(medir_ph_task_handle, pdFALSE);    /* Envía una notificación a la tarea asociada a medir agua*/
}

/**
 * @brief Funcion del Timer B que llama a las tareas cada 5 segundos
 */
void FuncTimer5seg(){
	vTaskNotifyGiveFromISR(notificar_task_handle, pdFALSE); /* Envía una notificación a la tarea asociada a notificar*/
}

/**
 * @brief Tarea que mide el nivel de la humedad y prende la bomba si es necesario
 */
static void TareaMedirAgua(void *pvParameter){
	while (encendido){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY); /* La tarea espera en este punto hasta recibir una notificación */
		humedad=GPIORead(GPIO_9);
		if(humedad==1){
			GPIOOn(GPIO_18); //se prende la bomba de agua
		}
		if (humedad==0){//se apaga si la bumedad esta bien
			GPIOOff(GPIO_18);
		}
	}
}

/**
 * @brief Tarea que mide en el CH1 la entrada analogica el nivel de pH y prende las bombas segun la medicion
 */
static void TareaMedirpH(void *pvParameter){
	while (encendido){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY); /* La tarea espera en este punto hasta recibir una notificación */
		AnalogInputReadSingle(CH1,&valor_ph);//valor ph es la variable donde se guarda lo q midio (me lo da en mV)
		if (valor_ph<1286){
			GPIOOff(GPIO_19); //se apaga la bomba acida
			bomba_acida=0;
			GPIOOn(GPIO_20);
			bomba_basica=1;
		}
		if (valor_ph>1436){
			GPIOOff(GPIO_20);// se apaga la bomba basica
			bomba_basica=0;
			GPIOOn(GPIO_19);
			bomba_acida=1;
		}
	}
}
 
/**
 * @brief Tarea que notifica por UART el estado cada 5 segundos
 */
static void TareaNotificarUART(void *pvParameter){ //UART
	while (encendido){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY); /* La tarea espera en este punto hasta recibir una notificación */
		UartSendString(UART_PC, "pH: ");
		UartSendString(UART_PC, (char *)UartItoa(valor_ph, 10));
		if(humedad==0){
			UartSendString(UART_PC, ", humedad correcta. \r\n");
		}
		else UartSendString(UART_PC, ", humedad incorrecta. \r\n");
		if(bomba_agua==1){
			UartSendString(UART_PC, "Bomba de agua encendida. \r\n");
		}
		if(bomba_acida==1){
			UartSendString(UART_PC, "Bomba de pHA encendida. \r\n");
		}
		if(bomba_basica==1){
			UartSendString(UART_PC, "Bomba de pHB encendida. \r\n");
		}
	}
}

/**
 * @brief Interrupccion del switch 1, prende el programa
 */
void Switch1On()
{
	encendido = true;
}

/**
 * @brief Interrupccion del switch 2, apaga el programa
 */

void Switch2Off()
{
	encendido=false;
}

/*==================[external functions definition]==========================*/
void app_main(void){
	timer_config_t timer_mediciones = { //timer para medir
	.timer = TIMER_A,
	.period = CONFIG_MEDICION_PERIOD,
	.func_p = FuncTimer3seg,
	.param_p = NULL
	};
	
	timer_config_t timer_notificacion = { //timer para notificar
	.timer = TIMER_B,
	.period = CONFIG_AVISO_PERIOD,
	.func_p = FuncTimer5seg,
	.param_p = NULL
	};
	
	analog_input_config_t canal_1 ={
	.input = CH1,		//canal1
	.mode = ADC_SINGLE, //single porque solo va por interrupciones
	.func_p = NULL,		// solo para modo continuo
	.param_p = NULL,	// solo para modo continuo
	.sample_frec = 0	//no lo vamos a usar
	};

	GPIOInit(GPIO_9, GPIO_INPUT); //entrada de la humerdad
	GPIOInit(GPIO_18, GPIO_OUTPUT); //salida de bomba de agua
	GPIOInit(GPIO_19, GPIO_OUTPUT); //salida de la bomba acida
	GPIOInit(GPIO_20, GPIO_OUTPUT); //salida de la bomba basica

	AnalogInputInit(&canal_1);
	
	TimerInit(&timer_mediciones); // Inicializa el timer A
	TimerInit(&timer_notificacion); // Inicializa el timer B
	
	SwitchActivInt(SWITCH_1, &Switch1On, NULL); // Activa la interrupcion del switch 1
	SwitchActivInt(SWITCH_2, &Switch2Off, NULL); // Activa la interrupcion del switch 2

	xTaskCreate(&TareaMedirAgua, "Medir agua", 512, NULL, 5, &medir_agua_task_handle);
	xTaskCreate(&TareaMedirpH, "Medir pH", 512, NULL, 5, &medir_ph_task_handle);
	xTaskCreate(&TareaNotificarUART, "Notificar por UART", 512, NULL, 5, &notificar_task_handle);

	TimerStart(timer_mediciones.timer);
	TimerStart(timer_notificacion.timer);
}
/*==================[end of file]============================================*/