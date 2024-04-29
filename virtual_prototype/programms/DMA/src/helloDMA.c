#include <ov7670.h>
#include <stdio.h>
#include <swap.h>
#include <vga.h>

#define TEST2
#define SRAM_ADDR 0x0000000
#define SCREEN_ADDR 0x50000020

enum dma_param {
  DMA_READ_WRITE = 0,
  DMA_BUS_START_ADDR = 1,
  DMA_MEM_START_ADDR = 2,
  DMA_BLOCK_SIZE = 3,
  DMA_BURST_SIZE = 4
};

enum dma_status {
  DMA_READY = 0,
  DMA_BUSY = 1,
  DMA_ERROR = 2,
  DMA_FAIL = 3
};

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

static void set_dma_conf(enum dma_param param,uint32_t value){
  if(param == DMA_READ_WRITE) return;
  uint32_t regA,regB;
  regA = (param << 10) | (1 << 9);
  regB = value;
  asm volatile("l.nios_rrr r0,%[in1],%[in2],0xF"
               :: [in1] "r"(regA), [in2] "r"(regB));
}
 
static uint32_t get_dma_conf(enum dma_param param){
  uint32_t regA,result;
  regA = (param << 10) | (1 << 9);
  asm volatile("l.nios_rrr %[out1],%[in1],r0,0xF"
               : [out1] "=r"(result)
               : [in1] "r"(regA));
  return result;
}

static void dma_start(){
  uint32_t regA = (0b101 << 10) | (1 << 9);
  asm volatile("l.nios_rrr r0,%[in1],r0,0xF"
               :: [in1] "r"(regA));
}

static enum dma_status dma_status(){
  uint32_t regA = (0b101 << 10);
  uint32_t result;
  asm volatile("l.nios_rrr %[out1],%[in1],r0,0xF"
               : [out1] "=r"(result)
               : [in1] "r"(regA));
  return (enum dma_status)result;
}

int main () {
  volatile uint32_t result;
  volatile unsigned int *vga = (unsigned int *)0X50000020;
  camParameters camParams;
  vga_clear();

  #ifdef TEST1
    for (uint16_t i = 0; i < 512; ++i) {
      printf("Writing %d at %X\n", i, i);
      writeDMA(i, i);
      const uint32_t res = readDMA(i);
      printf("Read %d at %X\n", res, i);
    }
  #elif defined(TEST2)
    static enum dma_status status;

    while (1){
      // init test screen
      for (uint16_t i = 0; i < 511; i+=2) {
        writeDMA(i, 0xFFFFFFFF);
        writeDMA(i+1, 0x00000000);
      }
      set_dma_conf(DMA_BUS_START_ADDR, SCREEN_ADDR);
      set_dma_conf(DMA_MEM_START_ADDR, 0);
      set_dma_conf(DMA_BLOCK_SIZE, 512);
      set_dma_conf(DMA_BURST_SIZE, 0);
      printf("DMA configured, starting.\n");
      dma_start();
      printf("generating image while transfer\n");
      for (uint16_t i = 0; i < 511; i+=2) {
        writeDMA(i, 0x00000000);
        writeDMA(i+1, 0xFFFFFFFF);
      }
      printf("waiting on DMA\n");
      do{
        status = dma_status(); 
        if (status == DMA_ERROR) {
          printf("DMA error\n");
          break;
        }
      } while (status != DMA_READY);
      printf("DMA finished transfer,starting new one.\n");
      dma_start();
      printf("waiting on DMA\n");
      do{
        status = dma_status(); 
        if (status == DMA_ERROR) {
          printf("DMA error\n");
          break;
        }
      } while (status != DMA_READY);
    }
    
    

  #endif
}
