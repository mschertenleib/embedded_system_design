#include <ov7670.h>
#include <stdio.h>
#include <swap.h>
#include <vga.h>

static void writeDMA(uint16_t addr, uint32_t data) {
  const uint32_t regA = (addr & 0x1ff) | 0x200;
  const uint32_t regB = data;
  uint32_t result;
  asm volatile("l.nios_rrr %[out1],%[in1],%[in2],0xF"
               : [out1] "=r"(result)
               : [in1] "r"(regA), [in2] "r"(regB));
}

static uint32_t readDMA(uint16_t addr) {
  const uint32_t regA = addr & 0x1ff;
  const uint32_t regB = 0;
  uint32_t result;
  asm volatile("l.nios_rrr %[out1],%[in1],%[in2],0xF"
               : [out1] "=r"(result)
               : [in1] "r"(regA), [in2] "r"(regB));
  return result;
}

int main() {
  vga_clear();

  for (uint16_t i = 0; i < 512; ++i) {
    writeDMA(i, i);
    const uint32_t res = readDMA(i);
    printf("wrote %d at %X and read %d", i, i, res);
  }
}
