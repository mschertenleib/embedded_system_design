#include <ov7670.h>
#include <stdio.h>
#include <string.h>
#include <swap.h>
#include <vga.h>

// ************** Configure gradient computation **************************

// Stream 2-bit gradients directly from the camera (assumes the verilog
// camera module is setup for streaming gradients).
//#define USE_GRAD_STREAMING

// Get 8-bit grayscale from the camera, convert them to gradients, using DMA for
// memory transfers (assumes the verilog camera module is setup for streaming
// 8-bit grayscale). Optionally, USE_CI_FOR_GRAD can also be defined to do the
// conversion using a CI. If not defined, everything is done in pure C (memory
// transfers and conversion). Only relevant if USE_GRAD_STREAMING is not
// defined.
#define USE_DMA_FOR_GRAD

// Convert grayscale to gradients using the conversion CI (assumes the
// verilog camera module is setup for streaming 8-bit grayscale). Can only be
// activated together with USE_DMA_FOR_GRAD, and only relevant if
// USE_GRAD_STREAMING is not defined.
#define USE_CI_FOR_GRAD

#define GRAD_THRESHOLD 10

// **************** Configure flow computation ****************************

// Display binary gradients as colors on the screen, and do not perform flow
// computation.
//#define DISPLAY_GRAD

// Compute flow using DMA for memory transfers and CIs for conversions. Only
// relevant if DISPLAY_GRAD is not defined.
#define USE_DMA_FOR_FLOW

// Compute flow using CIs for conversions, but do memory transfers using pure C
// array accesses. Only relevant if DISPLAY_GRAD and USE_DMA_FOR_FLOW are not
// defined.
#define USE_OPTIC_FLOW_CI

// If none of the three above symbols are defined, flow computation is done in
// pure C, without CIs or DMA.

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
  // Binary gradients, double buffered for current and previous frame
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

    uint32_t *grad_bin = grad_buffers[current_buffer];
    uint32_t *prev_grad_bin = grad_buffers[1 - current_buffer];

#ifdef USE_GRAD_STREAMING
    // Binary gradient is directly computed in streaming in the camera module
    takeSingleImageBlocking((uint32_t)grad_bin);

#else
    // We get 8-bit grayscale values from the camera module
    takeSingleImageBlocking((uint32_t)&grayscale[0]);

    // Reset counters
    asm volatile("l.nios_rrr r0,r0,%[in2],0xC" ::[in2] "r"(7));

    // Convert grayscale to binary gradients

#ifdef USE_DMA_FOR_GRAD

    asm volatile(
        "l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(burstSize | writeBit),
        [in2] "r"(39));

    for (int row = 1; row < 479; ++row) {
      // Read grayscale pixels into CI memory
      asm volatile(
          "l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(blockSize | writeBit),
          [in2] "r"(480));
      asm volatile(
          "l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(busStartAddress |
                                                       writeBit),
          [in2] "r"((uint32_t)&grayscale[((row - 1) << 9) + ((row - 1) << 7)]));
      asm volatile("l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(
                       memoryStartAddress | writeBit),
                   [in2] "r"(0));
      asm volatile("l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(statusControl |
                                                                writeBit),
                   [in2] "r"(1));
      waitDMA();

      uint32_t gray_up_u32;
      uint32_t gray_center_u32;
      uint32_t gray_down_u32;
      uint32_t gray_center_prev_u32;
      uint32_t gray_center_next_u32;
      asm volatile("l.nios_rrr %[out1],%[in1],r0,20"
                   : [out1] "=r"(gray_center_prev_u32)
                   : [in1] "r"(160));
      asm volatile("l.nios_rrr %[out1],%[in1],r0,20"
                   : [out1] "=r"(gray_center_u32)
                   : [in1] "r"(161));
      gray_center_prev_u32 = swap_u32(gray_center_prev_u32);
      gray_center_u32 = swap_u32(gray_center_u32);

      uint32_t grad_bin_u32 = 0;

      for (int base_index = 1; base_index < 159; ++base_index) {

        // Read grayscale pixels from CI memory to CPU registers
        asm volatile("l.nios_rrr %[out1],%[in1],r0,20"
                     : [out1] "=r"(gray_up_u32)
                     : [in1] "r"(base_index));
        asm volatile("l.nios_rrr %[out1],%[in1],r0,20"
                     : [out1] "=r"(gray_center_next_u32)
                     : [in1] "r"(160 + base_index + 1));
        asm volatile("l.nios_rrr %[out1],%[in1],r0,20"
                     : [out1] "=r"(gray_down_u32)
                     : [in1] "r"(320 + base_index));
        gray_up_u32 = swap_u32(gray_up_u32);
        gray_center_next_u32 = swap_u32(gray_center_next_u32);
        gray_down_u32 = swap_u32(gray_down_u32);

        // Iterate over each of the four 8-bit grayscale pixels of the 32-bit
        // words.
        for (int i = 0; i < 4; ++i) {

#ifdef USE_CI_FOR_GRAD
          // Do the grayscale to binary gradient conversion
          uint8_t gray_left = (i == 0)
                                  ? (gray_center_prev_u32 >> 24) & 0xff
                                  : (gray_center_u32 >> ((i - 1) << 3)) & 0xff;
          uint8_t gray_right = (i == 3)
                                   ? gray_center_next_u32 & 0xff
                                   : (gray_center_u32 >> ((i + 1) << 3)) & 0xff;
          uint8_t gray_up = (gray_up_u32 >> (i << 3)) & 0xff;
          uint8_t gray_down = (gray_down_u32 >> (i << 3)) & 0xff;

          uint32_t dx_dy;
          uint32_t d_u_r_l = ((uint32_t)gray_down << 24) |
                             ((uint32_t)gray_up << 16) |
                             ((uint32_t)gray_right << 8) | (uint32_t)gray_left;
          asm volatile("l.nios_rrr %[out1],%[in1],r0,0x32"
                       : [out1] "=r"(dx_dy)
                       : [in1] "r"(d_u_r_l));

          // Write 2-bit gradient into 32bit 16-gradient word
          const int bit_index = 30 - (((base_index & 0b11) << 2) + i) << 1;
          grad_bin_u32 =
              (grad_bin_u32 & ~(0b11 << bit_index)) | (dx_dy << bit_index);

#else
          // Do the grayscale to binary gradient conversion
          uint8_t gray_left = (i == 0)
                                  ? (gray_center_prev_u32 >> 24) & 0xff
                                  : (gray_center_u32 >> ((i - 1) << 3)) & 0xff;
          uint8_t gray_right = (i == 3)
                                   ? gray_center_next_u32 & 0xff
                                   : (gray_center_u32 >> ((i + 1) << 3)) & 0xff;
          uint8_t gray_up = (gray_up_u32 >> (i << 3)) & 0xff;
          uint8_t gray_down = (gray_down_u32 >> (i << 3)) & 0xff;

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

          // Write 2-bit gradient into 32bit 16-gradient word
          const int bit_index = 30 - (((base_index & 0b11) << 2) + i) << 1;
          grad_bin_u32 = (grad_bin_u32 & ~(0b11 << bit_index)) |
                         (dx << bit_index) | (dy << (bit_index + 1));
#endif
        }

        // Write gradients into CI memory
        if (base_index & 0b11 == 0b11) {
          asm volatile("l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(
                           writeBit | (base_index >> 2)),
                       [in2] "r"(grad_bin_u32));
        }

        gray_center_prev_u32 = gray_center_u32;
        gray_center_u32 = gray_center_next_u32;
      }

      // Store gradients to buffer
      asm volatile(
          "l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(blockSize | writeBit),
          [in2] "r"(40));
      asm volatile("l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(
                       busStartAddress | writeBit),
                   [in2] "r"((uint32_t)&grad_bin[(row << 5) + (row << 3)]));
      asm volatile("l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(
                       memoryStartAddress | writeBit),
                   [in2] "r"(0));
      asm volatile("l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(statusControl |
                                                                writeBit),
                   [in2] "r"(2));
      waitDMA();
    }
#else
    // Skip first and last rows and cols
    for (int pixel_index = camParams.nrOfPixelsPerLine;
         pixel_index <
         (camParams.nrOfLinesPerImage - 1) * camParams.nrOfPixelsPerLine;
         ++pixel_index) {

      // Compute binary gradients
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

      // Write 2-bit gradient into 32bit 16-gradient word buffer
      const int base_bin_index = pixel_index >> 4;
      const int bit_index = (pixel_index & 15) << 1;
      grad_bin[base_bin_index] =
          (grad_bin[base_bin_index] & ~(0b11 << bit_index)) |
          (dx << bit_index) | (dy << (bit_index + 1));
    }
#endif

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

    // Convert binary gradients to optic flow, or simply display gradients

#ifdef DISPLAY_GRAD

    for (int base_index = 0;
         base_index <
         ((camParams.nrOfLinesPerImage - 1) * camParams.nrOfPixelsPerLine) >> 4;
         ++base_index) {
      for (int j = 0; j < 16; ++j) {
        const int pixel_index = (base_index << 4) + j;
        const int bit_index = (pixel_index & 15) << 1;

        uint16_t pixel = 0;
        if ((grad_bin[base_index] >> bit_index) & 1)
          pixel |= swap_u16(0xf800);
        if ((grad_bin[base_index] >> (bit_index + 1)) & 1)
          pixel |= swap_u16(0x07e0);
        rgb565[pixel_index] = pixel;
      }
    }

#elif defined(USE_DMA_FOR_FLOW)

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

      // Read gradients from current image into CI memory
      asm volatile(
          "l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(blockSize | writeBit),
          [in2] "r"(80));
      asm volatile("l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(
                       busStartAddress | writeBit),
                   [in2] "r"((uint32_t)&grad_bin[(row << 5) + (row << 3)]));
      asm volatile("l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(
                       memoryStartAddress | writeBit),
                   [in2] "r"(0));
      asm volatile("l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(statusControl |
                                                                writeBit),
                   [in2] "r"(1));
      waitDMA();

      // Read gradients from previous image into CI memory
      asm volatile(
          "l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(blockSize | writeBit),
          [in2] "r"(80));
      asm volatile(
          "l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(busStartAddress |
                                                       writeBit),
          [in2] "r"((uint32_t)&prev_grad_bin[(row << 5) + (row << 3)]));
      asm volatile("l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(
                       memoryStartAddress | writeBit),
                   [in2] "r"(80));
      asm volatile("l.nios_rrr r0,%[in1],%[in2],20" ::[in1] "r"(statusControl |
                                                                writeBit),
                   [in2] "r"(1));
      waitDMA();

      for (int base_index = 0; base_index < 40; ++base_index) {

        // Read the 16*2bit = 32bit gradient values for 16 pixels for the four
        // rows {up, down, previous up, previous down}, from CI memory to CPU
        // registers.
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

        // Do the gradients to flow conversion, 8 pixels = 8*2bits at a time
        // (because the result of the CI is 32 bits, and each pixel flow is 4
        // bits for the 4 combinable directions).
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

        // Convert 4-bit flow directions to RGB565 colors for visualization, and
        // store them in the CI memory. In total, this represents 16 pixels (4
        // times 2 pixels for each of the following for-loops).
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

      // Transfer RGB565 pixels to the framebuffer.
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

      // Read the 16*2bit = 32bit gradient values for 16 pixels for the four
      // rows {up, down, previous up, previous down}, from CI memory to CPU
      // registers.
      const uint32_t row_up = grad_bin[base_index];
      const uint32_t row_down = grad_bin[base_index_next_row];
      const uint32_t prev_row_up = prev_grad_bin[base_index];
      const uint32_t prev_row_down = prev_grad_bin[base_index_next_row];

      // Do the gradients to flow conversion, 8 pixels = 8*2bits at a time
      // (because the result of the CI is 32 bits, and each pixel flow is 4
      // bits for the 4 combinable directions).
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

      // Do the gradient to flow conversion for 16 pixels at a time (16*2bit
      // inputs for each row).
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

      // Convert 4*1bit flow directions to RGB565 colors and store them in the
      // framebuffer.
      for (int j = 0; j < 16; ++j) {
        const int pixel_index = (base_index << 4) + j;
        const int bit_index = (pixel_index & 15) << 1;

        // 1-bit flow direction
        uint8_t left_flow = (left >> bit_index) & 1;
        uint8_t right_flow = (right >> bit_index) & 1;
        uint8_t up_flow = (up >> (bit_index + 1)) & 1;
        uint8_t down_flow = (down >> (bit_index + 1)) & 1;

        const uint8_t red = (left_flow << 4) | (down_flow << 4);
        const uint8_t green = (right_flow << 5) | (down_flow << 5);
        const uint8_t blue = (up_flow << 4) | (down_flow << 4);
        rgb565[pixel_index] = swap_u16((red << 11) | (green << 5) | blue);
      }
    }
#endif

    // Double buffering index switching.
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
