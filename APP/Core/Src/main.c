#include "main.h"
#include "uart_printf.h"
#include "stdio.h" //NULL
#include "shell.h"
#include "rcc.h"
#include "crc.h"
uint32_t SystemCoreClock = 72000000; //bootloader 已经设置了系统时钟，暂不需要重设所有外设

int main(void)
{

  if (init_HSE_SYSCLK_72MHZ() == true)
  {
    init_AHB_APB1_APB2();
    SystemCoreClock = 72000000;
  }
  else
  {
    uart_printf(standard_log, "init_HSE_SYSCLK_72MHZ fail......................\r\n", NULL, 0);
  }
  // init_AHB_APB1_APB2();
  uart_printf(standard_log, "NEW APP running......................\r\n", NULL, 0);

  crc_init();
  shell_init();
  uart2_init();
  uint8_t shell_data = 0;
  while (1)
  {

    if (get_frame_count() != 0)
    {
      uart2_communicate_rx();
      set_frame_count(get_frame_count() - 1);
    }
    if (uart_get(&shell_data) != false)
    {
      shell_enter(shell_data);
    }
  }
}