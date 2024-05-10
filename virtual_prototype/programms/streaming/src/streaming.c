#include <ov7670.h>
#include <stdio.h>
#include <swap.h>
#include <vga.h>

#define DMA       // Do transfers with DMA
#define STREAMING // Streaming
//    #define __RGB565__

static void waitDMA(void) {
  uint32_t status;
  do {
    asm volatile("l.nios_rrr %[out1],%[in1],r0,20"
                 : [out1] "=r"(status)
                 : [in1] "r"(5 << 10));
  } while (status & 1);
}

int main() {

  const uint8_t sevenSeg[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66,
                                0x6D, 0x7D, 0x07, 0x7F, 0x6F};
  volatile uint16_t rgb565[640 * 480];
  volatile uint8_t grayscale[640 * 480];
  volatile uint32_t result, cycles, stall, idle;
  volatile unsigned int *vga = (unsigned int *)0X50000020;
  volatile unsigned int *gpio = (unsigned int *)0x40000000;
  camParameters camParams;
  vga_clear();

  printf("Initialising camera (this takes up to 3 seconds)!\n");
  camParams = initOv7670(VGA);
  printf("Done!\n");
  printf("NrOfPixels : %d\n", camParams.nrOfPixelsPerLine);
  result = (camParams.nrOfPixelsPerLine <= 320)
               ? camParams.nrOfPixelsPerLine | 0x80000000
               : camParams.nrOfPixelsPerLine;
  vga[0] = swap_u32(result);
  printf("NrOfLines  : %d\n", camParams.nrOfLinesPerImage);
  result = (camParams.nrOfLinesPerImage <= 240)
               ? camParams.nrOfLinesPerImage | 0x80000000
               : camParams.nrOfLinesPerImage;
  vga[1] = swap_u32(result);
  printf("PCLK (kHz) : %d\n", camParams.pixelClockInkHz);
  printf("FPS        : %d\n", camParams.framesPerSecond);
  uint32_t grayPixels;
#if defined(STREAMING) && defined(__RGB565__)
  vga[2] = swap_u32(1);
  vga[3] = swap_u32((uint32_t)&rgb565[0]);
  enableContinues((uint32_t)&rgb565[0]);
#else
  vga[2] = swap_u32(2);
  vga[3] = swap_u32((uint32_t)&grayscale[0]);
#if defined(STREAMING)
  enableContinues((uint32_t)&grayscale[0]);
#endif
#endif

#if defined(STREAMING)

  while (1) {
  }

#else

  const uint32_t writeBit = 1 << 9;
  const uint32_t busStartAddress = 1 << 10;
  const uint32_t memoryStartAddress = 2 << 10;
  const uint32_t blockSize = 3 << 10;
  const uint32_t burstSize = 4 << 10;
  const uint32_t statusControl = 5 << 10;

  while (1) {

    takeSingleImageBlocking((uint32_t)&rgb565[0]);
    asm volatile("l.nios_rrr r0,r0,%[in2],0xC" ::[in2] "r"(7));

#ifdef DMA

    uint32_t *rgb = (uint32_t *)&rgb565[0];
    uint32_t *gray = (uint32_t *)&grayscale[0];

    asm volatile(
        "l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(burstSize | writeBit),
        [in2] "r"(15));
    asm volatile(
        "l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(blockSize | writeBit),
        [in2] "r"(256));
    asm volatile("l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(busStartAddress |
                                                              writeBit),
                 [in2] "r"(rgb));
    asm volatile("l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(
                     memoryStartAddress | writeBit),
                 [in2] "r"(0));
    asm volatile(
        "l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(statusControl | writeBit),
        [in2] "r"(1));

    waitDMA();

    for (int i = 0; i < 600; i++) {

      const uint32_t ramAddressTransfer = (i & 1) ? 0 : 256;
      const uint32_t ramAddressConvert = (i & 1) ? 256 : 0;

      if (i < 599) {
        // Initialize transfer to the second buffer
        rgb += 256;
        asm volatile(
            "l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(blockSize | writeBit),
            [in2] "r"(256));
        asm volatile("l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(
                         busStartAddress | writeBit),
                     [in2] "r"(rgb));
        asm volatile("l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(
                         memoryStartAddress | writeBit),
                     [in2] "r"(ramAddressTransfer));
        asm volatile("l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(
                         statusControl | writeBit),
                     [in2] "r"(1));
      }

      // Convert values in first buffer
      for (int j = 0; j < 128; j++) {
        // asm volatile("l.nios_rrr r0,r0,%[in2],0xC" ::[in2] "r"(7));

        // Read 4 pixels from the buffer
        uint32_t pixels1;
        uint32_t pixels2;
        asm volatile("l.nios_rrr %[out1],%[in1],r0,20"
                     : [out1] "=r"(pixels1)
                     : [in1] "r"(ramAddressConvert + (j << 1)));
        asm volatile("l.nios_rrr %[out1],%[in1],r0,20"
                     : [out1] "=r"(pixels2)
                     : [in1] "r"(ramAddressConvert + (j << 1) + 1));

        // Do the conversion
        pixels1 = swap_u32(pixels1);
        pixels2 = swap_u32(pixels2);
        asm volatile("l.nios_rrr %[out1],%[in1],%[in2],0xD"
                     : [out1] "=r"(grayPixels)
                     : [in1] "r"(pixels1), [in2] "r"(pixels2));
        grayPixels = swap_u32(grayPixels);

        // Write the grayscale values to the buffer
        asm volatile("l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(
                         writeBit | (ramAddressConvert + j)),
                     [in2] "r"(grayPixels));

        /*asm volatile("l.nios_rrr %[out1],r0,%[in2],0xC"
                     : [out1] "=r"(cycles)
                     : [in2] "r"(1 << 8 | 7 << 4));
        asm volatile("l.nios_rrr %[out1],%[in1],%[in2],0xC"
                     : [out1] "=r"(stall)
                     : [in1] "r"(1), [in2] "r"(1 << 9));
        asm volatile("l.nios_rrr %[out1],%[in1],%[in2],0xC"
                     : [out1] "=r"(idle)
                     : [in1] "r"(2), [in2] "r"(1 << 10));
        printf("inner: %d %d %d\n", cycles, stall, idle);*/
      }

      if (i < 599) {
        //  Wait for the other transfer to finish
        waitDMA();
      }

      // Start a transfer to the grayscale screen buffer
      asm volatile(
          "l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(blockSize | writeBit),
          [in2] "r"(128));
      asm volatile("l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(
                       busStartAddress | writeBit),
                   [in2] "r"(gray));
      asm volatile("l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(
                       memoryStartAddress | writeBit),
                   [in2] "r"(ramAddressConvert));
      asm volatile("l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(statusControl |
                                                                writeBit),
                   [in2] "r"(2));
      gray += 128;

      // Wait for the transfer to finish
      waitDMA();
    }

#else

    uint32_t *rgb = (uint32_t *)&rgb565[0];
    uint32_t *gray = (uint32_t *)&grayscale[0];
    for (int pixel = 0;
         pixel <
         ((camParams.nrOfLinesPerImage * camParams.nrOfPixelsPerLine) >> 1);
         pixel += 2) {
      uint32_t pixel1 = rgb[pixel];
      uint32_t pixel2 = rgb[pixel + 1];
      asm volatile("l.nios_rrr %[out1],%[in1],%[in2],0xD"
                   : [out1] "=r"(grayPixels)
                   : [in1] "r"(pixel1), [in2] "r"(pixel2));
      gray[0] = grayPixels;
      gray++;
    }

#endif

    asm volatile("l.nios_rrr %[out1],r0,%[in2],0xC"
                 : [out1] "=r"(cycles)
                 : [in2] "r"(1 << 8 | 7 << 4));
    asm volatile("l.nios_rrr %[out1],%[in1],%[in2],0xC"
                 : [out1] "=r"(stall)
                 : [in1] "r"(1), [in2] "r"(1 << 9));
    asm volatile("l.nios_rrr %[out1],%[in1],%[in2],0xC"
                 : [out1] "=r"(idle)
                 : [in1] "r"(2), [in2] "r"(1 << 10));
    printf("nrOfCycles: %d %d %d\n", cycles, stall, idle);
  }

#endif
}
