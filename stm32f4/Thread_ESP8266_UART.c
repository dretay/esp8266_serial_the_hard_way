#include "cmsis_os.h"                   // ARM::CMSIS:RTOS:Keil RTX
#include "stm32f4xx_hal.h"
#include "RTE_Device.h"
#include "Board_Buttons.h"
#include "stm32f4xx_hal_uart.h" 
#include <string.h>
#include <stdbool.h>
#include "stdlib.h"


extern void write_to_display(char* input);

void Thread_ESP8266_UART (void const *argument);                 // thread function
osThreadId tid_Thread_ESP8266_UART;                              // thread id
osThreadDef(Thread_ESP8266_UART, osPriorityNormal, 1, 0);        // thread object

#define USARTx                           USART1
#define USARTx_CLK_ENABLE()              __HAL_RCC_USART1_CLK_ENABLE();
#define DMAx_CLK_ENABLE()                __HAL_RCC_DMA2_CLK_ENABLE()
#define USARTx_RX_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOA_CLK_ENABLE()
#define USARTx_TX_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOA_CLK_ENABLE() 

#define USARTx_FORCE_RESET()             __HAL_RCC_USART1_FORCE_RESET()
#define USARTx_RELEASE_RESET()           __HAL_RCC_USART1_RELEASE_RESET()

/* Definition for USARTx Pins */
#define USARTx_TX_PIN                    GPIO_PIN_9
#define USARTx_TX_GPIO_PORT              GPIOA
#define USARTx_TX_AF                     GPIO_AF7_USART1
#define USARTx_RX_PIN                    GPIO_PIN_10
#define USARTx_RX_GPIO_PORT              GPIOA
#define USARTx_RX_AF                     GPIO_AF7_USART1

/* Definition for USARTx's DMA */
#define USARTx_TX_DMA_CHANNEL            DMA_CHANNEL_4
#define USARTx_TX_DMA_STREAM             DMA2_Stream7
#define USARTx_RX_DMA_CHANNEL            DMA_CHANNEL_4
#define USARTx_RX_DMA_STREAM             DMA2_Stream2

/* Definition for USARTx's NVIC */
#define USARTx_DMA_TX_IRQn               DMA2_Stream7_IRQn
#define USARTx_DMA_RX_IRQn               DMA2_Stream2_IRQn
#define USARTx_DMA_TX_IRQHandler         DMA2_Stream7_IRQHandler
#define USARTx_DMA_RX_IRQHandler         DMA2_Stream2_IRQHandler
#define USARTx_IRQn                      USART1_IRQn
#define USARTx_IRQHandler                USART1_IRQHandler

/* Size of Transmission buffer */
#define TXBUFFERSIZE                     (COUNTOF1(uartTxBuffer) - 1)
/* Size of Reception buffer */
#define RXBUFFERSIZE                     TXBUFFERSIZE

#define COUNTOF1(__BUFFER__)   (sizeof(__BUFFER__) / sizeof(*(__BUFFER__)))
	
UART_HandleTypeDef UartHandle;
__IO ITStatus UartReady = RESET;

/* Buffer used for transmission */
uint8_t uartTxBuffer[] = " ****UART_TwoBoards communication based on DMA****  ****UART_TwoBoards communication based on DMA****  ****UART_TwoBoards communication based on DMA**** ";

/* Buffer used for reception */
uint8_t uartRxBuffer[]= "                                                                                                                                                         ";
#define MAXCLISTRING          12 // Biggest string the user will type

uint8_t rxBuffer = '\000'; // where we store that one character that just came in
uint8_t rxString[MAXCLISTRING]; // where we build our string from characters coming in
int rxindex = 0; // index for going though rxString

int Init_UART(void) {
	static DMA_HandleTypeDef hdma_tx;
  static DMA_HandleTypeDef hdma_rx;
  
  GPIO_InitTypeDef  GPIO_InitStruct;
	
	UartHandle.Instance          = USARTx;
  
  UartHandle.Init.BaudRate     = 115200;
  UartHandle.Init.WordLength   = UART_WORDLENGTH_8B;
  UartHandle.Init.StopBits     = UART_STOPBITS_1;
  UartHandle.Init.Parity       = UART_PARITY_NONE;
  UartHandle.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
  UartHandle.Init.Mode         = UART_MODE_TX_RX;
  UartHandle.Init.OverSampling = UART_OVERSAMPLING_16;
    
  
  /*##-1- Enable peripherals and GPIO Clocks #################################*/
  /* Enable GPIO TX/RX clock */
  USARTx_TX_GPIO_CLK_ENABLE();
  USARTx_RX_GPIO_CLK_ENABLE();
  /* Enable USART1 clock */
  USARTx_CLK_ENABLE(); 
  /* Enable DMA1 clock */
  DMAx_CLK_ENABLE();   
  
  /*##-2- Configure peripheral GPIO ##########################################*/  
  /* UART TX GPIO pin configuration  */
  GPIO_InitStruct.Pin       = USARTx_TX_PIN;
  GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull      = GPIO_NOPULL;
  GPIO_InitStruct.Speed     = GPIO_SPEED_FAST;
  GPIO_InitStruct.Alternate = USARTx_TX_AF;
  
  HAL_GPIO_Init(USARTx_TX_GPIO_PORT, &GPIO_InitStruct);
    
  /* UART RX GPIO pin configuration  */
  GPIO_InitStruct.Pin = USARTx_RX_PIN;
  GPIO_InitStruct.Alternate = USARTx_RX_AF;
    
  HAL_GPIO_Init(USARTx_RX_GPIO_PORT, &GPIO_InitStruct);
    
  /*##-3- Configure the DMA streams ##########################################*/
  /* Configure the DMA handler for Transmission process */
  hdma_tx.Instance                 = USARTx_TX_DMA_STREAM;
  
  hdma_tx.Init.Channel             = USARTx_TX_DMA_CHANNEL;
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
  
  /* Associate the initialized DMA handle to the the UART handle */
  __HAL_LINKDMA(&UartHandle, hdmatx, hdma_tx);
    
  /* Configure the DMA handler for Transmission process */
  hdma_rx.Instance                 = USARTx_RX_DMA_STREAM;
  
  hdma_rx.Init.Channel             = USARTx_RX_DMA_CHANNEL;
  hdma_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
  hdma_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
  hdma_rx.Init.MemInc              = DMA_MINC_ENABLE;
  hdma_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  hdma_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
  hdma_rx.Init.Mode                = DMA_CIRCULAR;// DMA_NORMAL;
  hdma_rx.Init.Priority            = DMA_PRIORITY_HIGH;
  hdma_rx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;         
  hdma_rx.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
  hdma_rx.Init.MemBurst            = DMA_MBURST_INC4;
  hdma_rx.Init.PeriphBurst         = DMA_PBURST_INC4; 

  HAL_DMA_Init(&hdma_rx);
    
  /* Associate the initialized DMA handle to the the UART handle */
  __HAL_LINKDMA(&UartHandle, hdmarx, hdma_rx);
    
  /*##-4- Configure the NVIC for DMA #########################################*/
  /* NVIC configuration for DMA transfer complete interrupt (USARTx_TX) */
  HAL_NVIC_SetPriority(USARTx_DMA_TX_IRQn, 0, 1);
  HAL_NVIC_EnableIRQ(USARTx_DMA_TX_IRQn);
    
  /* NVIC configuration for DMA transfer complete interrupt (USARTx_RX) */
  HAL_NVIC_SetPriority(USARTx_DMA_RX_IRQn, 0, 0);   
  HAL_NVIC_EnableIRQ(USARTx_DMA_RX_IRQn);
  
  /* NVIC configuration for USART TC interrupt */
  HAL_NVIC_SetPriority(USARTx_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(USARTx_IRQn);
	
	
  if(HAL_UART_Init(&UartHandle) != HAL_OK){return(0);}
	return(1);

}
int Init_Thread_ESP8266_UART(void) {

	//initialize the SPI bus
	//todo: this should accept an argument to configure as master or slave?
	if (!Init_UART()) return (-1);

	tid_Thread_ESP8266_UART = osThreadCreate(osThread(Thread_ESP8266_UART), NULL);
  if (!tid_Thread_ESP8266_UART) return(-2);

  return(0);
}

void Thread_ESP8266_UART (void const *argument) {

	if(HAL_UART_Receive_DMA(&UartHandle, &rxBuffer, 1) != HAL_OK){}

  while (1) {
		
//  
//		/*##-3- Wait for the end of the transfer ###################################*/  
		//while (UartReady != SET){}

//		/* Reset transmission flag */
		//UartReady = RESET;
			//HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout)
			//HAL_UART_Receive(&UartHandle, (uint8_t *)uartRxBuffer, 153, 5000);
		
  }
}
/**
  * @brief  Tx Transfer completed callback
  * @param  UartHandle: UART handle. 
  * @note   This example shows a simple way to report end of DMA Tx transfer, and 
  *         you can add your own implementation. 
  * @retval None
  */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *UartHandle)
{
  /* Set transmission flag: transfer complete */
  UartReady = SET;



}

/**
  * @brief  Rx Transfer completed callback
  * @param  UartHandle: UART handle
  * @note   This example shows a simple way to report end of DMA Rx transfer, and 
  *         you can add your own implementation.
  * @retval None
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *UartHandle)
{
   //__HAL_UART_FLUSH_DRREGISTER(&huart1); // Clear the buffer to prevent overrun

    int i = 0;

    //print(&rxBuffer); // Echo the character that caused this callback so the user can see what they are typing

    if (rxBuffer == 8 || rxBuffer == 127) // If Backspace or del
    {
        //print(" \b"); // "\b space \b" clears the terminal character. Remember we just echoced a \b so don't need another one here, just space and \b
        rxindex--; 
        if (rxindex < 0) rxindex = 0;
    }

    else if (rxBuffer == '\n' || rxBuffer == '\r') // If Enter
    {
       // executeSerialCommand(rxString);
        rxString[rxindex] = 0;
        rxindex = 0;
        for (i = 0; i < MAXCLISTRING; i++) rxString[i] = 0; // Clear the string buffer
    }

    else
    {
        rxString[rxindex] = rxBuffer; // Add that character to the string
        rxindex++;
        if (rxindex > MAXCLISTRING) // User typing too much, we can't have commands that big
        {
            rxindex = 0;
            for (i = 0; i < MAXCLISTRING; i++) rxString[i] = 0; // Clear the string buffer
            //print("\r\nConsole> ");
        }
    }
}
/**
  * @brief  This function handles DMA RX interrupt request.  
  * @param  None
  * @retval None   
  */
void USARTx_DMA_RX_IRQHandler(void)
{
  HAL_DMA_IRQHandler(UartHandle.hdmarx);
}

/**
  * @brief  This function handles DMA TX interrupt request.
  * @param  None
  * @retval None   
  */
void USARTx_DMA_TX_IRQHandler(void)
{
  HAL_DMA_IRQHandler(UartHandle.hdmatx);
}

/**
  * @brief  This function handles USARTx interrupt request.
  * @param  None
  * @retval None
  */
void USARTx_IRQHandler(void)
{
  HAL_UART_IRQHandler(&UartHandle);
}
