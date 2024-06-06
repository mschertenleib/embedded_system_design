#include <ov7670.h>
#include <stdio.h>
#include <string.h>
#include <swap.h>
#include <vga.h>

//#define USE_OPTIC_FLOW_CI
//#define USE_DMA
//#define DERIV_C
#define RAW_STREAM

#define GRAD_THRESHOLD 10

static void waitDMA(void) {
  uint32_t status;
  do {
    asm volatile("l.nios_rrr %[out1],%[in1],r0,20"
                 : [out1] "=r"(status)
                 : [in1] "r"(5 << 10));
  } while (status & 1);
}

int main() {
  // Output image (color shows flow)
  volatile uint16_t rgb565[640 * 480];
  // Input image
  volatile uint8_t grayscale[640 * 480];
  // Binary gradients
  uint32_t grad_buffers[2][640 / 32 * 480 * 2];

  uint32_t result, cycles, stall, idle;
  volatile unsigned int *vga = (unsigned int *)0X50000020;
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
  vga[2] = swap_u32(1);
  vga[3] = swap_u32((uint32_t)&rgb565[0]);

  int current_buffer = 0;

  const uint32_t writeBit = 1 << 9;
  const uint32_t busStartAddress = 1 << 10;
  const uint32_t memoryStartAddress = 2 << 10;
  const uint32_t blockSize = 3 << 10;
  const uint32_t burstSize = 4 << 10;
  const uint32_t statusControl = 5 << 10;

  while (1) {
    takeSingleImageBlocking((uint32_t)&grayscale[0]);
    // Reset counters
    asm volatile("l.nios_rrr r0,r0,%[in2],0xC" ::[in2] "r"(7));

    uint32_t *grad_bin = grad_buffers[current_buffer];
    uint32_t *prev_grad_bin = grad_buffers[1 - current_buffer];

    // Convert grayscale to binary gradients
    #ifdef DERIV_C
    // Skip first and last rows and cols
    for (int pixel_index = camParams.nrOfPixelsPerLine;
         pixel_index <
         (camParams.nrOfLinesPerImage - 1) * camParams.nrOfPixelsPerLine;
         ++pixel_index) {

      const uint8_t gray_left = grayscale[pixel_index - 1];
      const uint8_t gray_right = grayscale[pixel_index + 1];
      const uint8_t gray_up =
          grayscale[pixel_index - camParams.nrOfPixelsPerLine];
      const uint8_t gray_down =
          grayscale[pixel_index + camParams.nrOfPixelsPerLine];

      uint8_t dx;
      if (gray_right >= gray_left) {
        dx = gray_right - gray_left > GRAD_THRESHOLD;
      } else {
        dx = gray_left - gray_right > GRAD_THRESHOLD;
      }
      uint8_t dy;
      if (gray_up >= gray_down) {
        dy = gray_up - gray_down > GRAD_THRESHOLD;
      } else {
        dy = gray_down - gray_up > GRAD_THRESHOLD;
      }

      const int base_bin_index = pixel_index >> 4;
      const int bit_index = (pixel_index & 15) << 1;
      grad_bin[base_bin_index] =
          (grad_bin[base_bin_index] & ~(0b11 << bit_index)) |
          (dx << bit_index) | (dy << (bit_index + 1));
    }

    // Read counters
    asm volatile("l.nios_rrr %[out1],r0,%[in2],0xC"
                 : [out1] "=r"(cycles)
                 : [in2] "r"(1 << 8 | 7 << 4));
    asm volatile("l.nios_rrr %[out1],%[in1],%[in2],0xC"
                 : [out1] "=r"(stall)
                 : [in1] "r"(1), [in2] "r"(1 << 9));
    asm volatile("l.nios_rrr %[out1],%[in1],%[in2],0xC"
                 : [out1] "=r"(idle)
                 : [in1] "r"(2), [in2] "r"(1 << 10));
    printf("Grad: Cycles: %d Stall: %d Idle: %d\n", cycles, stall, idle);
    #endif
    // Reset counters
    asm volatile("l.nios_rrr r0,r0,%[in2],0xC" ::[in2] "r"(7));

    // Convert binary gradients to optic flow

#ifdef USE_DMA

    uint32_t *rgb565_as_u32 = (uint32_t *)&rgb565[0];

    // For each row except the last one:
    // - Read gradients from current image: 2 lines of 640 pixels, 2 bits each
    // (80 words) to address 0
    // - Read gradients from previous image: 2 lines of 640 pixels, 2 bits each
    // (80 words) to address 80
    // - Do the computation, saving the 640 * 16 bits output pixels (320 words)
    // from address 160
    // - Write output to SRAM

    asm volatile(
        "l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(burstSize | writeBit),
        [in2] "r"(19));

    for (int row = 0; row < 479; ++row) {

      asm volatile(
          "l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(blockSize | writeBit),
          [in2] "r"(80));
      asm volatile("l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(
                       busStartAddress | writeBit),
                   [in2] "r"(&grad_bin[(row << 5) + (row << 3)]));
      asm volatile("l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(
                       memoryStartAddress | writeBit),
                   [in2] "r"(0));
      asm volatile("l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(statusControl |
                                                                writeBit),
                   [in2] "r"(1));
      waitDMA();

      asm volatile(
          "l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(blockSize | writeBit),
          [in2] "r"(80));
      asm volatile("l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(
                       busStartAddress | writeBit),
                   [in2] "r"(&prev_grad_bin[(row << 5) + (row << 3)]));
      asm volatile("l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(
                       memoryStartAddress | writeBit),
                   [in2] "r"(80));
      asm volatile("l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(statusControl |
                                                                writeBit),
                   [in2] "r"(1));
      waitDMA();

      for (int base_index = 0; base_index < 40; ++base_index) {

        uint32_t row_up;
        uint32_t row_down;
        uint32_t prev_row_up;
        uint32_t prev_row_down;
        asm volatile("l.nios_rrr %[out1],%[in1],r0,20"
                     : [out1] "=r"(row_up)
                     : [in1] "r"(base_index));
        asm volatile("l.nios_rrr %[out1],%[in1],r0,20"
                     : [out1] "=r"(row_down)
                     : [in1] "r"(40 + base_index));
        asm volatile("l.nios_rrr %[out1],%[in1],r0,20"
                     : [out1] "=r"(prev_row_up)
                     : [in1] "r"(80 + base_index));
        asm volatile("l.nios_rrr %[out1],%[in1],r0,20"
                     : [out1] "=r"(prev_row_down)
                     : [in1] "r"(120 + base_index));
        row_up = swap_u32(row_up);
        row_down = swap_u32(row_down);
        prev_row_up = swap_u32(prev_row_up);
        prev_row_down = swap_u32(prev_row_down);

        uint32_t valueA;
        uint32_t valueB;
        valueA = ((row_up & 0xffff) << 16) | (row_down & 0xffff);
        valueB = ((prev_row_up & 0xffff) << 16) | (prev_row_down & 0xffff);
        asm volatile("l.nios_rrr %[out1],%[in1],%[in2],0x30"
                     : [out1] "=r"(result)
                     : [in1] "r"(valueA), [in2] "r"(valueB));
        const uint32_t flow_0 = result;

        valueA = (row_up & 0xffff0000) | ((row_down & 0xffff0000) >> 16);
        valueB =
            (prev_row_up & 0xffff0000) | ((prev_row_down & 0xffff0000) >> 16);
        asm volatile("l.nios_rrr %[out1],%[in1],%[in2],0x30"
                     : [out1] "=r"(result)
                     : [in1] "r"(valueA), [in2] "r"(valueB));
        const uint32_t flow_1 = result;

        int pixel_index = base_index << 3;
        for (int i = 0; i < 4; ++i) {
          asm volatile("l.nios_rrr %[out1],%[in1],%[in2],0x31"
                       : [out1] "=r"(result)
                       : [in1] "r"(flow_0), [in2] "r"(i));
          asm volatile("l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(
                           writeBit | (160 + pixel_index)),
                       [in2] "r"(result));
          ++pixel_index;
        }
        for (int i = 0; i < 4; ++i) {
          asm volatile("l.nios_rrr %[out1],%[in1],%[in2],0x31"
                       : [out1] "=r"(result)
                       : [in1] "r"(flow_1), [in2] "r"(i));
          asm volatile("l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(
                           writeBit | (160 + pixel_index)),
                       [in2] "r"(result));
          ++pixel_index;
        }
      }

      asm volatile(
          "l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(blockSize | writeBit),
          [in2] "r"(320));
      asm volatile("l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(
                       busStartAddress | writeBit),
                   [in2] "r"((uint32_t)&rgb565[(row << 9) + (row << 7)]));
      asm volatile("l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(
                       memoryStartAddress | writeBit),
                   [in2] "r"(160));
      asm volatile("l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(statusControl |
                                                                writeBit),
                   [in2] "r"(2));
      waitDMA();
    }

#elif defined(USE_OPTIC_FLOW_CI)
    uint32_t *rgb565_as_u32 = (uint32_t *)&rgb565[0];

    for (int base_index = 0;
         base_index <
         ((camParams.nrOfLinesPerImage - 1) * camParams.nrOfPixelsPerLine) >> 4;
         ++base_index) {

      const int base_index_next_row =
          base_index + (camParams.nrOfPixelsPerLine >> 4);

      const uint32_t row_up = grad_bin[base_index];
      const uint32_t row_down = grad_bin[base_index_next_row];
      const uint32_t prev_row_up = prev_grad_bin[base_index];
      const uint32_t prev_row_down = prev_grad_bin[base_index_next_row];

      uint32_t valueA;
      uint32_t valueB;
      valueA = ((row_up & 0xffff) << 16) | (row_down & 0xffff);
      valueB = ((prev_row_up & 0xffff) << 16) | (prev_row_down & 0xffff);
      asm volatile("l.nios_rrr %[out1],%[in1],%[in2],0x30"
                   : [out1] "=r"(result)
                   : [in1] "r"(valueA), [in2] "r"(valueB));
      const uint32_t flow_0 = result;

      valueA = (row_up & 0xffff0000) | ((row_down & 0xffff0000) >> 16);
      valueB =
          (prev_row_up & 0xffff0000) | ((prev_row_down & 0xffff0000) >> 16);
      asm volatile("l.nios_rrr %[out1],%[in1],%[in2],0x30"
                   : [out1] "=r"(result)
                   : [in1] "r"(valueA), [in2] "r"(valueB));
      const uint32_t flow_1 = result;

      int pixel_index = base_index << 3;
      for (int i = 0; i < 4; ++i) {
        asm volatile("l.nios_rrr %[out1],%[in1],%[in2],0x31"
                     : [out1] "=r"(result)
                     : [in1] "r"(flow_0), [in2] "r"(i));
        rgb565_as_u32[pixel_index] = swap_u32(result);
        ++pixel_index;
      }
      for (int i = 0; i < 4; ++i) {
        asm volatile("l.nios_rrr %[out1],%[in1],%[in2],0x31"
                     : [out1] "=r"(result)
                     : [in1] "r"(flow_1), [in2] "r"(i));
        rgb565_as_u32[pixel_index] = swap_u32(result);
        ++pixel_index;
      }
    }
#else
    for (int base_index = 0;
         base_index <
         ((camParams.nrOfLinesPerImage - 1) * camParams.nrOfPixelsPerLine) >> 4;
         ++base_index) {

      const uint32_t left_and =
          grad_bin[base_index] & (prev_grad_bin[base_index] >> 2);
      const uint32_t right_and =
          (grad_bin[base_index] >> 2) & prev_grad_bin[base_index];
      const uint32_t left = left_and & ~right_and;
      const uint32_t right = right_and & ~left_and;

      const int base_index_next_row =
          base_index + (camParams.nrOfPixelsPerLine >> 4);
      const uint32_t up_and =
          grad_bin[base_index] & prev_grad_bin[base_index_next_row];
      const uint32_t down_and =
          grad_bin[base_index_next_row] & prev_grad_bin[base_index];
      const uint32_t up = up_and & ~down_and;
      const uint32_t down = down_and & ~up_and;

      for (int j = 0; j < 16; ++j) {
        const int pixel_index = (base_index << 4) + j;
        const int bit_index = (pixel_index & 15) << 1;

        #ifdef RAW_STREAM
          uint8_t dthx = (grad_bin[base_index] >> bit_index) & 1;
          uint8_t dthy = (grad_bin[base_index] >> bit_index+1) & 1;
          const uint8_t red = dthx*31;
          const uint8_t green = dthy*31;
          const uint8_t blue = 0;
          rgb565[pixel_index] = swap_u16((red << 11) | (green << 5) | blue);
        #else
          // 1-bit flow direction
          uint8_t left_flow = (left >> bit_index) & 1;
          uint8_t right_flow = (right >> bit_index) & 1;
          uint8_t up_flow = (up >> (bit_index + 1)) & 1;
          uint8_t down_flow = (down >> (bit_index + 1)) & 1;

          const uint8_t red = (left_flow << 4) | (down_flow << 4);
          const uint8_t green = (right_flow << 5) | (down_flow << 5);
          const uint8_t blue = (up_flow << 4) | (down_flow << 4);
          rgb565[pixel_index] = swap_u16((red << 11) | (green << 5) | blue);
        #endif
      }
    }
#endif

    current_buffer = 1 - current_buffer;

    // Read counters
    asm volatile("l.nios_rrr %[out1],r0,%[in2],0xC"
                 : [out1] "=r"(cycles)
                 : [in2] "r"(1 << 8 | 7 << 4));
    asm volatile("l.nios_rrr %[out1],%[in1],%[in2],0xC"
                 : [out1] "=r"(stall)
                 : [in1] "r"(1), [in2] "r"(1 << 9));
    asm volatile("l.nios_rrr %[out1],%[in1],%[in2],0xC"
                 : [out1] "=r"(idle)
                 : [in1] "r"(2), [in2] "r"(1 << 10));
    printf("Flow: Cycles: %d Stall: %d Idle: %d\n", cycles, stall, idle);
  }
}
