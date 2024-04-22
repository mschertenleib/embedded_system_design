#include <stdio.h>
#include <vga.h>
#include <ov7670.h>
#include <swap.h>


static uint32_t DMACi(uint32_t regA, uint32_t regB) {
  uint32_t result;
  asm volatile("l.nios_rrr %[out1],%[in1],%[in2],0xF"
               : [out1] "=r"(result)
               : [in1] "r"(regA), [in2] "r"(regB));
  return result;
}

int main () {
  vga_clear();
  printf("Hello World!\n" );
}
