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
  printf("Initialising camera (this takes up to 3 seconds)!\n");
  camParams = initOv7670(VGA);
  printf("Done!\n");
  printf("NrOfPixels : %lu\n", camParams.nrOfPixelsPerLine);
  result = (camParams.nrOfPixelsPerLine <= 320)
               ? camParams.nrOfPixelsPerLine | 0x80000000
               : camParams.nrOfPixelsPerLine;
  vga[0] = swap_u32(result);
  printf("NrOfLines  : %lu\n", camParams.nrOfLinesPerImage);
  result = (camParams.nrOfLinesPerImage <= 240)
               ? camParams.nrOfLinesPerImage | 0x80000000
               : camParams.nrOfLinesPerImage;
  vga[1] = swap_u32(result);
  printf("PCLK (kHz) : %lu\n", camParams.pixelClockInkHz);
  printf("FPS        : %lu\n", camParams.framesPerSecond);

  static uint32_t res =0;

  for(uint16_t i = 0 ; i < 512 ; i = i + 1 ){
    printf("writing %d at 0x%X\n",i,i);
    writeDMA(i,i);
  }
  printf("trying to read\n");
  for(uint16_t i = 511 ; i >= 0 ; i = i - 1 ){
    res = readDMA(i);
    printf("read at 0x%X : ->%d\n",i,res);
  }
}
