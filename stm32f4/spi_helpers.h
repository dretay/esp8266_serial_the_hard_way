#ifndef _SPI_HELPERS_H
#define _SPI_HELPERS_H

#define _GPIO_PIN(a) GPIO_PIN_##a
#define GPIO_PIN(a) _GPIO_PIN(a)

#define _DMA_INSTANCE(a, b) DMA##a##_##Stream##b
#define DMA_INSTANCE(a, b) _DMA_INSTANCE(a, b)

#define _DMA_CHANNEL(a) DMA_CHANNEL_##a
#define DMA_CHANNEL(a) _DMA_CHANNEL(a)

#define _DMA_INSTANCE_IRQn(a, b) DMA##a##_##Stream##b##_IRQn
#define DMA_INSTANCE_IRQn(a, b) _DMA_INSTANCE_IRQn(a,b)

#define _SPI_FORCE_RESET(a) __HAL_RCC_SPI##a##_FORCE_RESET
#define SPI_FORCE_RESET(a) _SPI_FORCE_RESET(a)()

#define _SPI_RELEASE_RESET(a) __HAL_RCC_SPI##a##_RELEASE_RESET
#define SPI_RELEASE_RESET(a) _SPI_RELEASE_RESET(a)()

#define _GPIO_CLK_ENABLE(a) __HAL_RCC_GPIO##a##_CLK_ENABLE
#define GPIO_CLK_ENABLE(a) _GPIO_CLK_ENABLE(a)()

#define _SPI_CLK_ENABLE(a) __HAL_RCC_SPI##a##_CLK_ENABLE
#define SPI_CLK_ENABLE(a) _SPI_CLK_ENABLE(a)()

#define _DMA_CLK_ENABLE(a) __HAL_RCC_DMA##a##_CLK_ENABLE                           
#define DMA_CLK_ENABLE(a) _DMA_CLK_ENABLE(a)()

#define _SPI_GPIO_AF(a,b) GPIO_AF##a##_SPI##b
#define SPI_GPIO_AF(a,b) _SPI_GPIO_AF(a,b)

#endif
