#include <ov7670.h>
#include <stdalign.h>
#include <stdio.h>
#include <swap.h>
#include <vga.h>

#define CAMERA
#define SRAM_ADDR 0x0000000
#define SCREEN_ADDR 0x50000020

enum dma_param {
  DMA_READ_WRITE = 0,
  DMA_BUS_START_ADDR = 1,
  DMA_MEM_START_ADDR = 2,
  DMA_BLOCK_SIZE = 3,
  DMA_BURST_SIZE = 4
};

enum dma_status { DMA_READY = 0, DMA_BUSY = 1, DMA_ERROR = 2, DMA_FAIL = 3 };

inline static uint32_t rgb565Grayscale(uint32_t pixels_1_0,
                                       uint32_t pixels_3_2) {
  uint32_t result;
  asm volatile("l.nios_rrr %[out1],%[in1],%[in2],0xD"
               : [out1] "=r"(result)
               : [in1] "r"(pixels_1_0), [in2] "r"(pixels_3_2));
  return result;
}

inline static void rgb2gray_parallel(volatile uint8_t *result,
                                     const volatile uint16_t *px) {
  uint32_t *imgptr = (uint32_t *)px;
  uint32_t gray = rgb565Grayscale(imgptr[1], imgptr[0]);
  uint32_t *ptrres = (uint32_t *)result;
  *ptrres = gray;
}

static void writeDMA(uint16_t addr, uint32_t data) {
  uint32_t regA, regB, result;
  regA = addr | (1 << 9);
  asm volatile("l.nios_rrr r0,%[in1],%[in2],0xF"
               : [out1] "=r"(result)
               : [in1] "r"(regA), [in2] "r"(regB));
}

static uint32_t readDMA(uint16_t addr) {
  uint32_t regA, result, regB = 0;
  regA = addr & ~(1 << 9);
  // printf("regA = 0x%X",regA);
  asm volatile("l.nios_rrr %[out1],%[in1],%[in2],0xF"
               : [out1] "=r"(result)
               : [in1] "r"(regA), [in2] "r"(regB));
  return result;
}

static void set_dma_conf(enum dma_param param, uint32_t value) {
  if (param == DMA_READ_WRITE)
    return;
  uint32_t regA, regB;
  regA = (param << 10) | (1 << 9);
  regB = value;
  asm volatile(
      "l.nios_rrr r0,%[in1],%[in2],0xF" ::[in1] "r"(regA), [in2] "r"(regB));
}

static uint32_t get_dma_conf(enum dma_param param) {
  uint32_t regA, result;
  regA = (param << 10) | (1 << 9);
  asm volatile("l.nios_rrr %[out1],%[in1],r0,0xF"
               : [out1] "=r"(result)
               : [in1] "r"(regA));
  return result;
}

static void dma_start() {
  uint32_t regA = (0b101 << 10) | (1 << 9);
  asm volatile("l.nios_rrr r0,%[in1],r0,0xF" ::[in1] "r"(regA));
}

static enum dma_status dma_status() {
  uint32_t regA = (0b101 << 10);
  uint32_t result;
  asm volatile("l.nios_rrr %[out1],%[in1],r0,0xF"
               : [out1] "=r"(result)
               : [in1] "r"(regA));
  return (enum dma_status)result;
}

int main() {
  volatile uint32_t result;
  volatile unsigned int *vga = (unsigned int *)0X50000020;
  camParameters camParams;
  vga_clear();

#ifdef READ_WRITE_TEST
  for (uint16_t i = 0; i < 512; ++i) {
    printf("Writing %d at %X\n", i, i);
    writeDMA(i, i);
    const uint32_t res = readDMA(i);
    printf("Read %d at %X\n", res, i);
  }
#elif defined(DMA_WRITE_SCREEN)
  static enum dma_status status;

  while (1) {
    // init test screen
    for (uint16_t i = 0; i < 511; i += 2) {
      writeDMA(i, 0xFFFFFFFF);
      writeDMA(i + 1, 0x00000000);
    }
    set_dma_conf(DMA_BUS_START_ADDR, SCREEN_ADDR);
    set_dma_conf(DMA_MEM_START_ADDR, 0);
    set_dma_conf(DMA_BLOCK_SIZE, 512);
    set_dma_conf(DMA_BURST_SIZE, 0);
    printf("DMA configured, starting.\n");
    dma_start();
    printf("generating image while transfer\n");
    for (uint16_t i = 0; i < 511; i += 2) {
      writeDMA(i, 0x00000000);
      writeDMA(i + 1, 0xFFFFFFFF);
    }
    printf("waiting on DMA\n");
    do {
      status = dma_status();
      if (status == DMA_ERROR) {
        printf("DMA error\n");
        break;
      }
    } while (status != DMA_READY);
    printf("DMA finished transfer,starting new one.\n");
    dma_start();
    printf("waiting on DMA\n");
    do {
      status = dma_status();
      if (status == DMA_ERROR) {
        printf("DMA error\n");
        break;
      }
    } while (status != DMA_READY);
  }

#elif defined(DMASTATE)
  static enum dma_status status;
  do {
    status = dma_status();
    switch (status) {
    case DMA_READY:
      printf("DMA ready\n");
      break;
    case DMA_BUSY:
      printf("DMA busy\n");
      break;
    case DMA_ERROR:
      printf("DMA error\n");
      break;
    case DMA_FAIL:
      printf("DMA fail\n");
      break;
    default:
      break;
    }
  } while (status != DMA_ERROR);

#elif defined(CAMERA)

  alignas(uint32_t) volatile uint16_t rgb565[640 * 480];
  alignas(uint32_t) volatile uint8_t grayscale[640 * 480];
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
    // printf("Image taken\n");
    for (int px = 0;
         px < camParams.nrOfLinesPerImage * camParams.nrOfPixelsPerLine;
         px += 4) {
      rgb2gray_parallel(&grayscale[px], &rgb565[px]);
    }
  }
#endif
}
