#include <stdio.h>
#include <vga.h>
#include <ov7670.h>
#include <swap.h>


static void writeDMA(uint16_t addr,uint32_t data) {
  uint32_t regA,result;
  regA = addr | (1 << 9);
  asm volatile("l.nios_rrr %[out1],%[in1],%[in2],0xF"
               : [out1] "=r"(result)
               : [in1] "r"(regA), [in2] "r"(data));
}

static uint32_t readDMA(uint16_t addr){
  uint32_t regA,result,regB=0;
  regA = addr & !(1 << 9);
  asm volatile("l.nios_rrr %[out1],%[in1],%[in2],0xF"
               : [out1] "=r"(result)
               : [in1] "r"(regA), [in2] "r"(regB));
  return result;
}

int main () {
  static uint32_t res =0;
  vga_clear();
  for(uint16_t i = 0 ; i < 512 ; i = i + 1 ){
    writeDMA(i,i);
    res = readDMA(i);
    printf("wrote %d at %X and read %d",i,i,res);
  }
}
