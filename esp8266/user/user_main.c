#include "ets_sys.h"
#include "osapi.h"
#include "user_interface.h"
#include "gpio.h"
#include "driver/uart.h"
#include "driver/spi.h"
#include "driver/i2c.h"

extern int ets_uart_printf(const char *fmt, ...);

#define user_procTaskPrio        0
#define user_procTaskQueueLen    32
os_event_t    user_procTaskQueue[user_procTaskQueueLen];

void ICACHE_FLASH_ATTR spi_msg_handler(os_event_t *e){
  uint8 cmd;

  ets_uart_printf("in signal handler\r");

  if(e->sig == 0x00){
    cmd = ((uint8*)e->par)[0];
    switch(cmd){
      case 0:
        ets_uart_printf("received command 0\n\r");
        uart1_tx_buffer ("hello world!",12);
        break;
      case 1:
        ets_uart_printf("received command 1\n\r");
        break;
      case 2:
        ets_uart_printf("received command 2\n\r");
        break;
      case 3:
        ets_uart_printf("received command 3\n\r");
        break;
      case 4:
        ets_uart_printf("received command 4\n\r");
        break;
      default:
        ets_uart_printf("unrecognized request %d\n\r",cmd);
        break;
    }
  }
}
void user_init(void){
    uint8_t i2c_read_val;
    // Configure the UART (br0, br1)
    uart_init(BIT_RATE_115200, BIT_RATE_115200);

    spi_slave_init(HSPI);
    system_os_task(spi_msg_handler, user_procTaskPrio,user_procTaskQueue, user_procTaskQueueLen);
    ets_uart_printf("bus init complete\n\r");

}
