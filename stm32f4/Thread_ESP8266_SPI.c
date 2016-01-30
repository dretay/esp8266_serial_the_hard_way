#include "spi_helpers.h"
#include "cmsis_os.h"                   // ARM::CMSIS:RTOS:Keil RTX
#include "stm32f4xx_hal.h"
#include "RTE_Device.h"
#include "Board_Buttons.h"
#include <string.h>
#include <stdbool.h>
#include "stdlib.h"
// ::Board Support:Buttons
#define COUNTOF(__BUFFER__)   (sizeof(__BUFFER__) / sizeof(*(__BUFFER__)))
#define BUFFERSIZE                       (COUNTOF(aTxBuffer))

typedef struct MyFirstStruct MyFirstStruct;
extern void write_to_display(char* input);

extern osMessageQId Q_SPI_CMD;

void Thread_ESP8266_SPI (void const *argument);                 // thread function
osThreadId tid_Thread_ESP8266_SPI;                              // thread id
osThreadDef(Thread_ESP8266_SPI, osPriorityNormal, 1, 1024);        // thread object
extern osMutexId GPIOB13_MUTEX;

#define MASTER_BOARD 1

SPI_HandleTypeDef SpiHandle;

/* Buffer used for transmission */
uint8_t aTxBuffer[2];
uint8_t aRxBuffer[BUFFERSIZE];

int Init_SPI_Bus(bool is_master) {
	GPIO_InitTypeDef GPIO_InitStruct;

#if RTE_SPI2_TX_DMA == 1
	static DMA_HandleTypeDef hdma_tx;
#endif
#if RTE_SPI2_RX_DMA == 1
  static DMA_HandleTypeDef hdma_rx;
#endif

	//
	// Enable bus clocks
	//
	GPIO_CLK_ENABLE(B);
	SPI_CLK_ENABLE(2);//todo: is this necessary?

#if RTE_SPI2_TX_DMA == 1
	DMA_CLK_ENABLE(RTE_SPI2_TX_DMA);
#endif
#if RTE_SPI2_RX_DMA == 1
	DMA_CLK_ENABLE(RTE_SPI2_RX_DMA);
#endif

	//
	// Configure spi pins
	//
	GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull      = GPIO_PULLDOWN;
	GPIO_InitStruct.Speed     = GPIO_SPEED_FAST;

	//SCK
	GPIO_InitStruct.Pin       = GPIO_PIN(RTE_SPI2_SCL_BIT);
	GPIO_InitStruct.Alternate = SPI_GPIO_AF(5,2);

	HAL_GPIO_Init(RTE_SPI2_SCL_PORT, &GPIO_InitStruct);
	HAL_GPIO_WritePin(RTE_SPI2_SCL_PORT, GPIO_PIN(RTE_SPI2_SCL_BIT), GPIO_PIN_RESET);

	//MISO
	GPIO_InitStruct.Pin = GPIO_PIN(RTE_SPI2_MISO_BIT);
	GPIO_InitStruct.Alternate = SPI_GPIO_AF(5,2);

	HAL_GPIO_Init(RTE_SPI2_MISO_PORT, &GPIO_InitStruct);
	HAL_GPIO_WritePin(RTE_SPI2_MISO_PORT, GPIO_PIN(RTE_SPI2_MISO_BIT), GPIO_PIN_RESET);

	//MOSI
	GPIO_InitStruct.Pin = GPIO_PIN(RTE_SPI2_MOSI_BIT);
	GPIO_InitStruct.Alternate = SPI_GPIO_AF(5,2);

	HAL_GPIO_Init(RTE_SPI2_MOSI_PORT, &GPIO_InitStruct);
	HAL_GPIO_WritePin(RTE_SPI2_MOSI_PORT, GPIO_PIN(RTE_SPI2_MOSI_BIT), GPIO_PIN_RESET);

		//
		//Configure DMA streams
		//

		//dma tx
#if RTE_SPI2_TX_DMA   == 1
		hdma_tx.Instance                 = DMA_INSTANCE(RTE_SPI2_TX_DMA_NUMBER,RTE_SPI2_TX_DMA_STREAM);
		hdma_tx.Init.Channel             = DMA_CHANNEL(RTE_SPI2_TX_DMA_CHANNEL);
		hdma_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
		hdma_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
		hdma_tx.Init.MemInc              = DMA_MINC_ENABLE;
		hdma_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
		hdma_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
		hdma_tx.Init.Mode                = DMA_NORMAL;
		hdma_tx.Init.Priority            = DMA_PRIORITY_LOW;
		hdma_tx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
		hdma_tx.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
		hdma_tx.Init.MemBurst            = DMA_MBURST_INC4;
		hdma_tx.Init.PeriphBurst         = DMA_PBURST_INC4;

		HAL_DMA_Init(&hdma_tx);

		//link spi handle to tx dma
		__HAL_LINKDMA(&SpiHandle, hdmatx, hdma_tx);

		//setup NVIC interrupt for dma complete interrupt
		HAL_NVIC_SetPriority(DMA_INSTANCE_IRQn(RTE_SPI2_TX_DMA_NUMBER,RTE_SPI2_TX_DMA_STREAM), 0, 1);
		HAL_NVIC_EnableIRQ(DMA_INSTANCE_IRQn(RTE_SPI2_TX_DMA_NUMBER,RTE_SPI2_TX_DMA_STREAM));

#endif
		//dma rx
#if RTE_SPI2_RX_DMA   == 1
		hdma_rx.Instance                 = DMA_INSTANCE(RTE_SPI2_RX_DMA_NUMBER,RTE_SPI2_RX_DMA_STREAM);
		hdma_rx.Init.Channel             = DMA_CHANNEL(RTE_SPI2_RX_DMA_CHANNEL);
		hdma_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
		hdma_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
		hdma_rx.Init.MemInc              = DMA_MINC_ENABLE;
		hdma_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
		hdma_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
		hdma_rx.Init.Mode                = DMA_NORMAL;
		hdma_rx.Init.Priority            = DMA_PRIORITY_HIGH;
		hdma_rx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
		hdma_rx.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
		hdma_rx.Init.MemBurst            = DMA_MBURST_INC4;
		hdma_rx.Init.PeriphBurst         = DMA_PBURST_INC4;

		HAL_DMA_Init(&hdma_rx);

		//link spi handle to rx dma
		__HAL_LINKDMA(&SpiHandle, hdmarx, hdma_rx);

		//setup NVIC interrupt for dma complete interrupt
		HAL_NVIC_SetPriority(DMA_INSTANCE_IRQn(RTE_SPI2_RX_DMA_NUMBER,RTE_SPI2_RX_DMA_STREAM), 0, 0);
		HAL_NVIC_EnableIRQ(DMA_INSTANCE_IRQn(RTE_SPI2_RX_DMA_NUMBER,RTE_SPI2_RX_DMA_STREAM));
#endif


  //
	// Configure spi bus
	//
	SpiHandle.Instance               = SPI2;
  SpiHandle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128;
  SpiHandle.Init.Direction         = SPI_DIRECTION_1LINE;
  SpiHandle.Init.CLKPhase          = SPI_PHASE_1EDGE;
  SpiHandle.Init.CLKPolarity       = SPI_POLARITY_LOW;
  SpiHandle.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
  SpiHandle.Init.CRCPolynomial     = 7;
  SpiHandle.Init.DataSize          = SPI_DATASIZE_8BIT;
  SpiHandle.Init.FirstBit          = SPI_FIRSTBIT_MSB;
  SpiHandle.Init.NSS               = SPI_NSS_SOFT;
  SpiHandle.Init.TIMode            = SPI_TIMODE_DISABLE;

	//todo: this should be a if / else from a passed in param rather than #ifdef'd
	if(is_master){
  	SpiHandle.Init.Mode = SPI_MODE_MASTER;
  }
	else{
  	SpiHandle.Init.Mode = SPI_MODE_SLAVE;
  }

  if(HAL_SPI_Init(&SpiHandle) != HAL_OK){return(0);}

	return(1);

}
int Init_Thread_ESP8266_SPI(void) {

	//initialize the SPI bus
	//todo: this should accept an argument to configure as master or slave?
	if (!Init_SPI_Bus(true)) return (-1);

	tid_Thread_ESP8266_SPI = osThreadCreate(osThread(Thread_ESP8266_SPI), NULL);
  if (!tid_Thread_ESP8266_SPI) return(-2);

  return(0);
}

void Thread_ESP8266_SPI (void const *argument) {

	int state =0;
	char msg[10];
	osEvent  result;


//	char* mystring;
//	char myrealstring[] ="hello";
//	mystring = (char*)malloc(10 * sizeof(char));
//	strncpy(mystring, myrealstring,5);


  while (1) {
	  //wait for button
		result = 	osMessageGet(Q_SPI_CMD,osWaitForever);

		aTxBuffer[0] = 0x20;
		aTxBuffer[1] = result.value.v;

		//acquire bus
		osMutexWait(GPIOB13_MUTEX, osWaitForever);

		 sprintf(msg,"sent cmd %d",result.value.v);
		 write_to_display(msg);

		//set cs pin low
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_RESET);

		//todo: figure out a better way to handle errors
		switch(HAL_SPI_Transmit_DMA(&SpiHandle, (uint8_t*)aTxBuffer, BUFFERSIZE)){
			case HAL_OK:
				break;
			case HAL_TIMEOUT:
				break;
			case HAL_ERROR:
				break;
			default:
				break;
		}


		//wait for the xfer to finish
		while (HAL_SPI_GetState(&SpiHandle) != HAL_SPI_STATE_READY){}


		// //set cs high
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_SET);




		osMutexRelease(GPIOB13_MUTEX);
  }
}

//these are hard-coded in rte_device for spi instance
#ifdef RTE_SPI2_RX_DMA
void DMA1_Stream3_IRQHandler(void){
  HAL_DMA_IRQHandler(SpiHandle.hdmarx);
}
#endif
#ifdef RTE_SPI2_TX_DMA
void DMA1_Stream4_IRQHandler(void){
  HAL_DMA_IRQHandler(SpiHandle.hdmatx);
}
#endif


//void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
//{
//  static DMA_HandleTypeDef hdma_tx;
//  static DMA_HandleTypeDef hdma_rx;
//
//  GPIO_InitTypeDef  GPIO_InitStruct;
//
//	SPI_CLK_ENABLE(2);
//	DMA_CLK_ENABLE(RTE_SPI2_RX_DMA);
//
//
//
//  /*##-2- Configure peripheral GPIO ##########################################*/
//  /* SPI SCK GPIO pin configuration  */
//  GPIO_InitStruct.Pin       = GPIO_PIN(RTE_SPI2_SCL_BIT); //SPIx_SCK_PIN;
//  GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
//  GPIO_InitStruct.Pull      = GPIO_PULLDOWN;
//  GPIO_InitStruct.Speed     = GPIO_SPEED_FAST;
//  GPIO_InitStruct.Alternate = SPI_GPIO_AF(5,2);
//
//  HAL_GPIO_Init(RTE_SPI2_SCL_PORT, &GPIO_InitStruct);
//	HAL_GPIO_WritePin(RTE_SPI2_SCL_PORT, GPIO_PIN(RTE_SPI2_SCL_BIT), GPIO_PIN_RESET);
//
//  /* SPI MISO GPIO pin configuration  */
//  GPIO_InitStruct.Pin = GPIO_PIN(RTE_SPI2_MISO_BIT);
//  GPIO_InitStruct.Alternate = SPI_GPIO_AF(5,2);
//
//  HAL_GPIO_Init(RTE_SPI2_MISO_PORT, &GPIO_InitStruct);
//	HAL_GPIO_WritePin(RTE_SPI2_MISO_PORT, GPIO_PIN(RTE_SPI2_MISO_BIT), GPIO_PIN_RESET);
//
//  /* SPI MOSI GPIO pin configuration  */
//  GPIO_InitStruct.Pin = GPIO_PIN(RTE_SPI2_MOSI_BIT);
//  GPIO_InitStruct.Alternate = SPI_GPIO_AF(5,2);
//
//  HAL_GPIO_Init(RTE_SPI2_MOSI_PORT, &GPIO_InitStruct);
//	HAL_GPIO_WritePin(RTE_SPI2_MOSI_PORT, GPIO_PIN(RTE_SPI2_MOSI_BIT), GPIO_PIN_RESET);
//
//  /*##-3- Configure the DMA streams ##########################################*/
//  /* Configure the DMA handler for Transmission process */
//	hdma_tx.Instance                 = DMA_INSTANCE(RTE_SPI2_TX_DMA_NUMBER,RTE_SPI2_TX_DMA_STREAM);
//  hdma_tx.Init.Channel             = DMA_CHANNEL(RTE_SPI2_TX_DMA_CHANNEL);
//  hdma_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
//  hdma_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
//  hdma_tx.Init.MemInc              = DMA_MINC_ENABLE;
//  hdma_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
//  hdma_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
//  hdma_tx.Init.Mode                = DMA_NORMAL;
//  hdma_tx.Init.Priority            = DMA_PRIORITY_LOW;
//  hdma_tx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
//  hdma_tx.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
//  hdma_tx.Init.MemBurst            = DMA_MBURST_INC4;
//  hdma_tx.Init.PeriphBurst         = DMA_PBURST_INC4;
//
//  HAL_DMA_Init(&hdma_tx);
//
//  /* Associate the initialized DMA handle to the the SPI handle */
//  __HAL_LINKDMA(hspi, hdmatx, hdma_tx);
//
//  /* Configure the DMA handler for Transmission process */
//  hdma_rx.Instance                 = DMA_INSTANCE(RTE_SPI2_RX_DMA_NUMBER,RTE_SPI2_RX_DMA_STREAM);
//
//  hdma_rx.Init.Channel             = DMA_CHANNEL(RTE_SPI2_RX_DMA_CHANNEL);
//  hdma_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
//  hdma_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
//  hdma_rx.Init.MemInc              = DMA_MINC_ENABLE;
//  hdma_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
//  hdma_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
//  hdma_rx.Init.Mode                = DMA_NORMAL;
//  hdma_rx.Init.Priority            = DMA_PRIORITY_HIGH;
//  hdma_rx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
//  hdma_rx.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
//  hdma_rx.Init.MemBurst            = DMA_MBURST_INC4;
//  hdma_rx.Init.PeriphBurst         = DMA_PBURST_INC4;

//  HAL_DMA_Init(&hdma_rx);
//
//  /* Associate the initialized DMA handle to the the SPI handle */
//  __HAL_LINKDMA(hspi, hdmarx, hdma_rx);
//
//  /*##-4- Configure the NVIC for DMA #########################################*/
//  /* NVIC configuration for DMA transfer complete interrupt (SPI3_TX) */
//  HAL_NVIC_SetPriority(DMA_INSTANCE_IRQn(RTE_SPI2_TX_DMA_NUMBER,RTE_SPI2_TX_DMA_STREAM), 0, 1);
//  HAL_NVIC_EnableIRQ(DMA_INSTANCE_IRQn(RTE_SPI2_TX_DMA_NUMBER,RTE_SPI2_TX_DMA_STREAM));
//
//  /* NVIC configuration for DMA transfer complete interrupt (SPI3_RX) */
//  HAL_NVIC_SetPriority(DMA_INSTANCE_IRQn(RTE_SPI2_RX_DMA_NUMBER,RTE_SPI2_RX_DMA_STREAM), 0, 0);
//  HAL_NVIC_EnableIRQ(DMA_INSTANCE_IRQn(RTE_SPI2_RX_DMA_NUMBER,RTE_SPI2_RX_DMA_STREAM));
//}

///**
//  * @brief SPI MSP De-Initialization
//  *        This function frees the hardware resources used in this example:
//  *          - Disable the Peripheral's clock
//  *          - Revert GPIO, DMA and NVIC configuration to their default state
//  * @param hspi: SPI handle pointer
//  * @retval None
//  */
//void HAL_SPI_MspDeInit(SPI_HandleTypeDef *hspi)
//{
//
//  static DMA_HandleTypeDef hdma_tx;
//  static DMA_HandleTypeDef hdma_rx;

//  /*##-1- Reset peripherals ##################################################*/
//  SPI_FORCE_RESET(2);
//  SPI_RELEASE_RESET(2);

//  /*##-2- Disable peripherals and GPIO Clocks ################################*/
//  /* Configure SPI SCK as alternate function  */
//  HAL_GPIO_DeInit(RTE_SPI2_SCL_PORT, GPIO_PIN(RTE_SPI2_SCL_BIT));
//  /* Configure SPI MISO as alternate function  */
//  HAL_GPIO_DeInit(RTE_SPI2_MISO_PORT, GPIO_PIN(RTE_SPI2_MISO_BIT));
//  /* Configure SPI MOSI as alternate function  */
//  HAL_GPIO_DeInit(RTE_SPI2_MOSI_PORT, GPIO_PIN(RTE_SPI2_MOSI_BIT));
//
//  /*##-3- Disable the DMA Streams ############################################*/
//  /* De-Initialize the DMA Stream associate to transmission process */
//  HAL_DMA_DeInit(&hdma_tx);
//  /* De-Initialize the DMA Stream associate to reception process */
//  HAL_DMA_DeInit(&hdma_rx);
//
//  /*##-4- Disable the NVIC for DMA ###########################################*/
//  HAL_NVIC_DisableIRQ(DMA_INSTANCE_IRQn(RTE_SPI2_TX_DMA_NUMBER,RTE_SPI2_TX_DMA_STREAM));
//  HAL_NVIC_DisableIRQ(DMA_INSTANCE_IRQn(RTE_SPI2_RX_DMA_NUMBER,RTE_SPI2_RX_DMA_STREAM));
//}
