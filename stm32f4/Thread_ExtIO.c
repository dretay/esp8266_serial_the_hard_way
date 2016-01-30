#include "cmsis_os.h"                   // ARM::CMSIS:RTOS:Keil RTX
#include "stm32f4xx_hal.h"
#include "spi_helpers.h"
#include <stdbool.h>


void Thread_ExtIO (void const *argument);                 // thread function
osThreadId tid_Thread_ExtIO;                              // thread id
osThreadDef(Thread_ExtIO, osPriorityNormal, 1, 0);        // thread object
bool ESP_PROGRAMMING = false;

extern osMessageQId Q_SPI_CMD;
extern osMutexId GPIOB13_MUTEX;
extern void write_to_display(char* input);


int Init_ExtIO(void){
	GPIO_InitTypeDef GPIO_InitStruct;

	//
	// Enable bus clocks
	//
	GPIO_CLK_ENABLE(A);
	GPIO_CLK_ENABLE(B);
	GPIO_CLK_ENABLE(C);


	//
	// Configure external io pins
	//
	GPIO_InitStruct.Pin = GPIO_PIN_0;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
	GPIO_InitStruct.Pull = GPIO_PULLDOWN;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

//	HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 1);
//	HAL_NVIC_EnableIRQ(EXTI0_IRQn);

	// esp-related pins 11 = cs, 12 = rst, 13=esp_gpio0
	GPIO_InitStruct.Pin   = GPIO_PIN_11 | GPIO_PIN_12 |GPIO_PIN_13;
	GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull  = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	//button-related pins
	GPIO_InitStruct.Pin   = GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8| GPIO_PIN_9;
	GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull  = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	osMutexWait(GPIOB13_MUTEX, osWaitForever);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
	write_to_display("PWR IN 3");
	HAL_Delay(1000);
	write_to_display("PWR IN 2");
	HAL_Delay(1000);
	write_to_display("PWR IN 1");
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
	HAL_Delay(1000);
	write_to_display("RUNNING");
	osMutexRelease(GPIOB13_MUTEX);

	return(1);



}
int Init_Thread_ExtIO(void) {
	if (!Init_ExtIO()) return (-1);
  tid_Thread_ExtIO= osThreadCreate(osThread(Thread_ExtIO), NULL);
  if (!tid_Thread_ExtIO) return(-1);

  return(0);
}

void Thread_ExtIO(void const *argument) {

  while (1) {
		//TODO: if in programming mode should it set itself back after a timeout?
		// this is the button on the board
		if (Buttons_GetState() & 1U){
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_RESET);
			if(!ESP_PROGRAMMING){
					write_to_display("PROGRAMMING");
					osMutexWait(GPIOB13_MUTEX, osWaitForever);
					HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
			}else{
				write_to_display("NORMAL");
				HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
			}
			HAL_Delay(1000);
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
			if(ESP_PROGRAMMING){
				osMutexRelease(GPIOB13_MUTEX);
			}
			ESP_PROGRAMMING = !ESP_PROGRAMMING;
		}
		if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_6) == GPIO_PIN_SET) {
			osMessagePut(Q_SPI_CMD,0x00,osWaitForever);
		}
		if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_7) == GPIO_PIN_SET) {
			osMessagePut(Q_SPI_CMD,0x01,osWaitForever);
		}
		if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_8) == GPIO_PIN_SET) {
			osMessagePut(Q_SPI_CMD,0x02,osWaitForever);
		}
		if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_9) == GPIO_PIN_SET) {
			osMessagePut(Q_SPI_CMD,0x03,osWaitForever);
		}
		HAL_Delay(200);
  }
}
//void EXTI0_IRQHandler(void){
//	osSignalSet(tid_Thread_ExtIO , 1U);
//	__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_0);
//	HAL_NVIC_ClearPendingIRQ(EXTI0_IRQn);
//}
