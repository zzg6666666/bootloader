#include "main.h"
#include "uart_printf.h"
#include "stdio.h" //NULL
#include "shell.h"
#include "rcc.h"
#include "crc.h"

uint32_t SystemCoreClock = 8000000;

void jump_to_app(void);

int main(void)
{

  // re set

  if (init_HSE_SYSCLK_72MHZ() == true)
  {
    init_AHB_APB1_APB2();
    SystemCoreClock = 72000000;
  }
  else
  {
    uart_printf(standard_log, "init_HSE_SYSCLK_72MHZ fail......\r\n", NULL, 0);
  }
  // init_AHB_APB1_APB2();
  uart_printf(standard_log, "bootloader running......\r\n", NULL, 0);

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

void jump_to_app()
{

#if 0 // 从异常向量表中，从储存reset_handle的位置开始跳转，需要设置最低位为1，告诉编译器是thumb指令
  // 中断向量表的地址
  unsigned int isr_vector_address = 0x8002800;

  // 取出中断向量表中，存储Reset_Handler代码段地址的地址(表中第二个就是)
  unsigned int temp = isr_vector_address + 0x4;

  // 将temp的LSB(最低位)设置为1，告诉CPU，这个执行的是thumb指令，而且temp指向的是Reset_Handler代码段地址
  temp |= 0x01;

  // 创建一个函数指针 返回类型是void ，参数类型是void；
  void (*APP)(void);

  APP = (void (*)(void))(temp);
  // 打印出APP在向量表中的地址
  uart_printf("APP address in isr_vector:", &temp, 4);
  // 不打印出内存,一行代码
  // void (*APP)(void) = (void (*)(void))(0x8002805);
  //
#else // 从reset_handle的程序地址，开始跳转

  // 定义 uint32_t的指针变量，该变量指向的地址是0x8002800
  /* 在汇编中isr_vector_address会被优化，会变成一个常量值存在ROM里面 */
  unsigned int *isr_vector_address = (unsigned int *)0x8002800;

  // 将Reset_Handler代码在flash的地址取出，放到temp中
  /* 在汇编中,以下代码会被优化成常量值，直接加载常量值到某个寄存器中 */
  unsigned int temp = *(isr_vector_address + 1);

  // 将APP的地址指向temp
  void (*APP)(void) = (void (*)(void))temp;

  // 打印出APP的地址
  uint8_t temp1[4];

  temp1[0] = temp >> 24;
  temp1[1] = temp >> 16;
  temp1[2] = temp >> 8;
  temp1[3] = temp >> 0;
  uart_printf(standard_log, "APP address is:", temp1, 4);

#endif
  // 执行Reset_Handler

  // 设置app的中断向量表的偏移量(在rom中)

  volatile uint32_t *SCB_VTOR = (uint32_t *)0xE000ED08;

  // 中断向量表在rom中的地址
  *SCB_VTOR = 0x8002800;
  // uint32_t VTCB_data = *SCB_VTOR;
  // uart_printf("VTCB_data  is :", &VTCB_data, 4);

  APP();
}