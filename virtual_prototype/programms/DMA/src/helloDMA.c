#include <ov7670.h>
#include <stdio.h>
#include <swap.h>
#include <vga.h>

static void writeDMA(uint16_t addr,uint32_t data) {
  uint32_t regA,regB,result;
  regA = addr | (1 << 9);
  asm volatile("l.nios_rrr r0,%[in1],%[in2],0xF"
               : [out1] "=r"(result)
               : [in1] "r"(regA), [in2] "r"(regB));
}

static uint32_t readDMA(uint16_t addr){
  uint32_t regA,result,regB=0;
  regA = addr & ~(1 << 9);
  //printf("regA = 0x%X",regA);
  asm volatile("l.nios_rrr %[out1],%[in1],%[in2],0xF"
               : [out1] "=r"(result)
               : [in1] "r"(regA), [in2] "r"(regB));
  return result;
}

int main () {
  volatile uint32_t result;
  volatile unsigned int *vga = (unsigned int *)0X50000020;
  camParameters camParams;
  vga_clear();

  for (uint16_t i = 0; i < 512; ++i) {
    printf("Writing %d at %X\n", i, i);
    writeDMA(i, i);
    const uint32_t res = readDMA(i);
    printf("Read %d at %X\n", res, i);
  }
}
