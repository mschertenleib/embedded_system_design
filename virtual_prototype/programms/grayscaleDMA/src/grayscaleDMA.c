#include <ov7670.h>
#include <swap.h>
#include <vga.h>
#include <stdalign.h>
#include <stdio.h>

#define PARALLEL// UNOPT,SINGLEPX,PARALLEL

typedef enum {
  COUNTER_CYCLES = 0,
  COUNTER_STALL = 1,
  COUNTER_BUS_IDLE = 2,
  COUNTER_CYCLES_2 = 3
} CounterType;

typedef enum {
  ENABLE_CYCLES = 0x001,
  ENABLE_STALL = 0x002,
  ENABLE_BUS_IDLE = 0x004,
  ENABLE_CYCLES_2 = 0x008,
  DISABLE_CYCLES = 0x010,
  DISABLE_STALL = 0x020,
  DISABLE_BUS_IDLE = 0x040,
  DISABLE_CYCLES_2 = 0x080,
  RESET_CYCLES = 0x100,
  RESET_STALL = 0x200,
  RESET_BUS_IDLE = 0x400,
  RESET_CYCLES_2 = 0x800
} CounterControlBits;

typedef enum{
  WRITE_BIT = 1<<9,
  BUS_START_ADDRESS = 1 << 10,
  MEMORY_START_ADDRESS = 2 << 10,
  BLOCK_SIZE = 3 << 10,
  BURST_SIZE = 4 << 10,
  STATUS_CONTROL = 5 << 10
} dmaConfig;

enum dma_status {
  DMA_READY = 0,
  DMA_BUSY = 1,
  DMA_ERROR = 2,
  DMA_FAIL = 3
};

static uint32_t read_counter(CounterType counterId) {
  uint32_t result;
  asm volatile("l.nios_rrr %[out1],%[in1],r0,0xC"
               : [out1] "=r"(result)
               : [in1] "r"(counterId));
  return result;
}

inline static uint32_t rgb565Grayscale(uint32_t pixels_1_0, uint32_t pixels_3_2) {
  uint32_t result;
  asm volatile("l.nios_rrr %[out1],%[in1],%[in2],0xD"
               : [out1] "=r"(result)
               : [in1] "r"(pixels_1_0), [in2] "r"(pixels_3_2));
  return result;
}

inline static uint8_t rgb2gray(uint16_t rgb565) {
  uint32_t px = (uint32_t)rgb565;
  return (uint8_t)rgb565Grayscale(px, 0);
}

inline static void rgb2gray_parallel(volatile uint8_t *result, const volatile uint16_t *px) {
  uint32_t* imgptr = (uint32_t*) px;
  uint32_t gray = rgb565Grayscale(imgptr[1], imgptr[0]);
  uint32_t* ptrres = (uint32_t*)result;
  *ptrres = gray;
}

static void control_counters(uint32_t control) {
  asm volatile("l.nios_rrr r0,r0,%[in2],0x0C" ::[in2] "r"(control));
}

static void dma_conf_trans(uint32_t *memBuffer, uint32_t usedCiRamAddress, uint32_t usedBlocksize, uint32_t usedBurstSize){
  asm volatile("l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(BUS_START_ADDRESS | WRITE_BIT),[in2] "r"(memBuffer));
  asm volatile("l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(MEMORY_START_ADDRESS | WRITE_BIT),[in2] "r"(usedCiRamAddress));
  asm volatile("l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(BLOCK_SIZE | WRITE_BIT),[in2] "r"(usedBlocksize));
  asm volatile("l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(BURST_SIZE | WRITE_BIT),[in2] "r"(usedBurstSize));
}

static void dma_start_trans(){
  asm volatile("l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(STATUS_CONTROL | WRITE_BIT),[in2] "r"(1));
}

static void grayscale_block_convert(uint8_t *grayBlockPtr,uint16_t dmaAddr){
  uint16_t imgBlockBuffer[256] = {0};
  uint32_t res;
  uint16_t px[4];
  for(uint16_t pxidx = 0; pxidx < 256; pxidx += 4){
    asm volatile("l.nios_rrr %[out1],%[in1],r0,20" :[out1]"=r"(res):[in1] "r"(dmaAddr+pxidx+0));
    px[0] = (uint16_t)res;
    asm volatile("l.nios_rrr %[out1],%[in1],r0,20" :[out1]"=r"(res):[in1] "r"(dmaAddr+pxidx+1));
    px[1] = (uint16_t)res;
    asm volatile("l.nios_rrr %[out1],%[in1],r0,20" :[out1]"=r"(res):[in1] "r"(dmaAddr+pxidx+2));
    px[2] = (uint16_t)res;
    asm volatile("l.nios_rrr %[out1],%[in1],r0,20" :[out1]"=r"(res):[in1] "r"(dmaAddr+pxidx+3));
    px[3] = (uint16_t)res;
    rgb2gray_parallel(grayBlockPtr[pxidx], px[0]);
  }
}

static uint8_t get_dma_status(){
  uint32_t status;
  asm volatile("l.nios_rrr %[out1],%[in1],r0,0x14" : [out1] "=r"(status): [in1] "r"(STATUS_CONTROL & ~WRITE_BIT));
  return (uint8_t)status;
}

static void wait_dma(){
  uint32_t status;
  do{
    status = get_dma_status();
  }while(status != DMA_READY);
}

static void grayscale_dma(uint8_t *grayBuffer,uint32_t *camAddr){
  dma_conf_trans(camAddr,0,256,256);
  dma_start_trans();
  for(int block = 0; block < 600; block+=2){
    dma_wait();                                             //wait for the first block transfer to finish
    dma_conf_trans(camAddr+block*256,256,256,256);          //configure the next transfer to second block of CiRam
    dma_start_trans();                                      //start the transfer from cam to CiRam
    grayscale_block_convert(grayBuffer+block*256,0);        //convert the first block of CiRam to grayscale

    dma_wait();                                             //wait for the second block transfer to finish                         
    dma_conf_trans(camAddr+(block+1)*256,0,256,256);        //configure the next transfer to first block of CiRam
    dma_start_trans();                                      //start the transfer from cam to CiRam
    grayscale_block_convert(grayBuffer+(block+1)*256,256);  //convert the second block of CiRam to grayscale
  }
  wait_dma();
  grayscale_block_convert(grayBuffer+600*256,0);
}

int main() {
  alignas(uint32_t) volatile uint16_t rgb565[640 * 480];
  alignas(uint32_t) volatile uint8_t grayscale[640 * 480];
  volatile uint32_t result, cycles, stall, idle;
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
  uint32_t *rgb = (uint32_t *)&rgb565[0];
  uint32_t grayPixels;
  vga[2] = swap_u32(2);
  vga[3] = swap_u32((uint32_t)&grayscale[0]);

  while (1) {
    uint32_t *gray = (uint32_t *)&grayscale[0];
    takeSingleImageBlocking((uint32_t)&rgb565[0]);
    control_counters(RESET_CYCLES | RESET_BUS_IDLE | RESET_STALL | RESET_CYCLES_2);
    control_counters(ENABLE_CYCLES | ENABLE_STALL | ENABLE_BUS_IDLE | ENABLE_CYCLES_2);
    //start of conversion
    grayscale_dma(&grayscale[0],vga);
    
    control_counters(DISABLE_CYCLES | DISABLE_BUS_IDLE | DISABLE_STALL | DISABLE_CYCLES_2);
    stall = read_counter(COUNTER_STALL);
    idle = read_counter(COUNTER_BUS_IDLE);
    cycles = read_counter(COUNTER_CYCLES);
    printf("C: %lu S: %lu BI: %lu Eff: %lu\n", cycles, stall, idle, cycles - stall);
  }
}