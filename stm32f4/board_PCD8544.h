/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

#ifndef _GDISP_LLD_BOARD_H
#define _GDISP_LLD_BOARD_H

#include "gfx.h"
#include "stm32f4xx_hal.h"
#include "RTE_Device.h"
#include "spi_helpers.h"

SPI_HandleTypeDef DisplaySpiHandle;


static GFXINLINE void init_board(GDisplay *g) {	
#if RTE_SPI3_TX_DMA == 1
	static DMA_HandleTypeDef hdma_tx;
#endif
#if RTE_SPI3_RX_DMA == 1 
	static DMA_HandleTypeDef hdma_rx;
#endif
	GPIO_InitTypeDef GPIO_InitStruct;

	// As we are not using multiple displays we set g->board to NULL as we don't use it.
	g->board = 0;
	
	switch(g->controllerdisplay) {
	case 0:															
	
		//
		// Enable bus clocks
		//
		SPI_CLK_ENABLE(3);
		GPIO_CLK_ENABLE(B);
		GPIO_CLK_ENABLE(D);

#if RTE_SPI3_RX_DMA  == 1
		DMA_CLK_ENABLE(RTE_SPI3_RX_DMA);
#endif
#if RTE_SPI3_TX_DMA  == 1
		DMA_CLK_ENABLE(RTE_SPI3_TX_DMA);
#endif

		//
		// Configure spi pins
		//
		GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull      = GPIO_PULLDOWN;
		GPIO_InitStruct.Speed     = GPIO_SPEED_FAST;
		
		//SCK  
		GPIO_InitStruct.Pin       = GPIO_PIN(RTE_SPI3_SCL_BIT); 
		GPIO_InitStruct.Alternate = SPI_GPIO_AF(6,3);
		
		HAL_GPIO_Init(RTE_SPI3_SCL_PORT, &GPIO_InitStruct);
		HAL_GPIO_WritePin(RTE_SPI3_SCL_PORT, GPIO_PIN(RTE_SPI3_SCL_BIT), GPIO_PIN_RESET);
			
		//MISO 
		GPIO_InitStruct.Pin = GPIO_PIN(RTE_SPI3_MISO_BIT);
		GPIO_InitStruct.Alternate = SPI_GPIO_AF(6,3);
		
		HAL_GPIO_Init(RTE_SPI3_MISO_PORT, &GPIO_InitStruct);
		HAL_GPIO_WritePin(RTE_SPI3_MISO_PORT, GPIO_PIN(RTE_SPI3_MISO_BIT), GPIO_PIN_RESET);
		
		//MOSI  
		GPIO_InitStruct.Pin = GPIO_PIN(RTE_SPI3_MOSI_BIT);
		GPIO_InitStruct.Alternate = SPI_GPIO_AF(6,3);
			
		HAL_GPIO_Init(RTE_SPI3_MOSI_PORT, &GPIO_InitStruct);
		HAL_GPIO_WritePin(RTE_SPI3_MOSI_PORT, GPIO_PIN(RTE_SPI3_MOSI_BIT), GPIO_PIN_RESET);
		
		//    
		//Configure DMA streams
		//
	
		//dma tx
#if RTE_SPI3_TX_DMA   == 1
		hdma_tx.Instance                 = DMA_INSTANCE(RTE_SPI3_TX_DMA_NUMBER,RTE_SPI3_TX_DMA_STREAM);
		hdma_tx.Init.Channel             = DMA_CHANNEL(RTE_SPI3_TX_DMA_CHANNEL);
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
		__HAL_LINKDMA(&DisplaySpiHandle, hdmatx, hdma_tx);
		
		//setup NVIC interrupt for dma complete interrupt
		HAL_NVIC_SetPriority(DMA_INSTANCE_IRQn(RTE_SPI3_TX_DMA_NUMBER,RTE_SPI3_TX_DMA_STREAM), 0, 1);
		HAL_NVIC_EnableIRQ(DMA_INSTANCE_IRQn(RTE_SPI3_TX_DMA_NUMBER,RTE_SPI3_TX_DMA_STREAM));
	
#endif   
		//dma rx
#if RTE_SPI3_RX_DMA   == 1
		hdma_rx.Instance                 = DMA_INSTANCE(RTE_SPI3_RX_DMA_NUMBER,RTE_SPI3_RX_DMA_STREAM);		
		hdma_rx.Init.Channel             = DMA_CHANNEL(RTE_SPI3_RX_DMA_CHANNEL);
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
		__HAL_LINKDMA(&DisplaySpiHandle, hdmarx, hdma_rx);
		
		//setup NVIC interrupt for dma complete interrupt
		HAL_NVIC_SetPriority(DMA_INSTANCE_IRQn(RTE_SPI3_RX_DMA_NUMBER,RTE_SPI3_RX_DMA_STREAM), 0, 0);   
		HAL_NVIC_EnableIRQ(DMA_INSTANCE_IRQn(RTE_SPI3_RX_DMA_NUMBER,RTE_SPI3_RX_DMA_STREAM));
#endif

		//
		// Configure spi bus
		//
		DisplaySpiHandle.Instance               = SPI3;
		DisplaySpiHandle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128;
		DisplaySpiHandle.Init.Direction         = SPI_DIRECTION_1LINE;
		DisplaySpiHandle.Init.CLKPhase          = SPI_PHASE_1EDGE;
		DisplaySpiHandle.Init.CLKPolarity       = SPI_POLARITY_LOW;
		DisplaySpiHandle.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
		DisplaySpiHandle.Init.CRCPolynomial     = 7;
		DisplaySpiHandle.Init.DataSize          = SPI_DATASIZE_8BIT;
		DisplaySpiHandle.Init.FirstBit          = SPI_FIRSTBIT_MSB;
		DisplaySpiHandle.Init.NSS               = SPI_NSS_SOFT;
		DisplaySpiHandle.Init.TIMode            = SPI_TIMODE_DISABLE;
		DisplaySpiHandle.Init.Mode 							= SPI_MODE_MASTER;

		if(HAL_SPI_Init(&DisplaySpiHandle) != HAL_OK){/*how should i handle this error?*/}		
		else{
		
			//D3 = bk light, D4 = RST, D5 = CE, D6 = DC
			GPIO_InitStruct.Pin   = GPIO_PIN_3| GPIO_PIN_4| GPIO_PIN_5 | GPIO_PIN_6; 
			GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
			GPIO_InitStruct.Pull  = GPIO_PULLUP;
			GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
			HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);	

			//set cs high
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_5, GPIO_PIN_SET); 
			
			//set backlight on
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_5, GPIO_PIN_RESET); 
			
		}
        
		break;
	}
}

static GFXINLINE void post_init_board(GDisplay *g) {
	(void) g;
	

}

static GFXINLINE void setpin_reset(GDisplay *g, bool_t state) {
	(void) g;
	(void) state;
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, state ? GPIO_PIN_RESET : GPIO_PIN_SET);		
}

static GFXINLINE void set_backlight(GDisplay *g, uint8_t percent) {
	(void) g;
	(void) percent;
	if (percent == 0){
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3, GPIO_PIN_SET); 
	}
	else{
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3, GPIO_PIN_RESET); 
	}
}

static GFXINLINE void acquire_bus(GDisplay *g) {
	(void) g;
	
	
}

static GFXINLINE void release_bus(GDisplay *g) {
	(void) g;
	
	
}

static GFXINLINE void write_cmd(GDisplay *g, uint8_t cmd) {
	(void) g;
	
	//set dc pin low
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_6, GPIO_PIN_RESET);		
	
	//set cs low		
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_5, GPIO_PIN_RESET); 
	
	
	//todo: figure out a better way to handle errors
#if RTE_SPI3_TX_DMA  == 1
	if(HAL_SPI_Transmit_DMA(&DisplaySpiHandle, &cmd, 1) != HAL_OK){}

	//wait for the xfer to finish
	while (HAL_SPI_GetState(&DisplaySpiHandle) != HAL_SPI_STATE_READY){}

#else		
	if(HAL_SPI_Transmit(&DisplaySpiHandle, &cmd, sizeof(uint8_t), 100) != HAL_OK){}
#endif
		
	//set cs pin high
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_5, GPIO_PIN_SET);		
	
}

static GFXINLINE void write_data(GDisplay *g, uint8_t* data, uint16_t length) {	
	(void) g;	

	//set dc pin high
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_6, GPIO_PIN_SET);	
	
	//set cs low
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_5, GPIO_PIN_RESET); 
	

#if RTE_SPI3_TX_DMA  == 1
	if(HAL_SPI_Transmit_DMA(&DisplaySpiHandle, data, length) != HAL_OK){}

	//wait for the xfer to finish
	while (HAL_SPI_GetState(&DisplaySpiHandle) != HAL_SPI_STATE_READY){}

#else			
		if(HAL_SPI_Transmit(&DisplaySpiHandle, data, length, 100) != HAL_OK){}	
#endif
	
	
	//set cs pin high	
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_5, GPIO_PIN_SET);				
	
}
//
//DMA interrupt handlers
//  note: these are hard-coded in rte_device for spi instance
//
#if RTE_SPI3_RX_DMA  == 1
void DMA1_Stream0_IRQHandler(void){
  HAL_DMA_IRQHandler(DisplaySpiHandle.hdmarx);
}
#endif
#if RTE_SPI3_TX_DMA  == 1
void DMA1_Stream5_IRQHandler(void){
  HAL_DMA_IRQHandler(DisplaySpiHandle.hdmatx);
}
#endif



/**
* TODO: what should i do with this? how can i free resources on the display?
  */
//void HAL_SPI_MspDeInit(SPI_HandleTypeDef *hspi)
//{
//  
//#if RTE_SPI3_TX_DMA  == 1
//  static DMA_HandleTypeDef hdma_tx;
//#endif
//#if RTE_SPI3_RX_DMA  == 1
//  static DMA_HandleTypeDef hdma_rx;
//#endif
//  
//	//
//  // Reset spi bus
//	//
//  SPI_FORCE_RESET(3);
//  SPI_RELEASE_RESET(3);
//	
//	//
//	// Disable clocks and set pins to alternate function
//	//
//  HAL_GPIO_DeInit(RTE_SPI3_SCL_PORT, GPIO_PIN(RTE_SPI3_SCL_BIT));  
//  HAL_GPIO_DeInit(RTE_SPI3_MISO_PORT, GPIO_PIN(RTE_SPI3_MISO_BIT));  
//  HAL_GPIO_DeInit(RTE_SPI3_MOSI_PORT, GPIO_PIN(RTE_SPI3_MOSI_BIT));
//   
//	//
//	// Disable DMA streams and NVIC interrupts
//	//
//#if RTE_SPI3_TX_DMA  == 1
//  HAL_DMA_DeInit(&hdma_tx); 
//	HAL_NVIC_DisableIRQ(DMA_INSTANCE_IRQn(RTE_SPI2_TX_DMA_NUMBER,RTE_SPI2_TX_DMA_STREAM));
//#endif
//#if RTE_SPI3_RX_DMA   == 1
//  HAL_DMA_DeInit(&hdma_rx);
//	HAL_NVIC_DisableIRQ(DMA_INSTANCE_IRQn(RTE_SPI2_RX_DMA_NUMBER,RTE_SPI2_RX_DMA_STREAM));
//#endif  
//  
//}

#endif /* _GDISP_LLD_BOARD_H */
