#include <ov7670.h>
#include <stdio.h>
#include <string.h>
#include <swap.h>
#include <vga.h>

#define GRAD_THRESHOLD 20

int main() {
  const uint8_t sevenSeg[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66,
                                0x6D, 0x7D, 0x07, 0x7F, 0x6F};

  // Output image (color shows flow)
  volatile uint16_t rgb565[640 * 480];
  // Input image
  volatile uint8_t grayscale[640 * 480];
  // Binary gradients
  volatile uint32_t grad_bin[640 / 32 * 480 * 2];
  volatile uint32_t prev_grad_bin[640 / 32 * 480 * 2];

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
  vga[2] = swap_u32(1);
  vga[3] = swap_u32((uint32_t)&rgb565[0]);

  while (1) {
    takeSingleImageBlocking((uint32_t)&grayscale[0]);
    // Reset counters
    asm volatile("l.nios_rrr r0,r0,%[in2],0xC" ::[in2] "r"(7));

    // Convert grayscale to binary gradients

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

    // Convert binary gradients to optic flow

    // Skip the last row because we need to compute a difference between two
    // rows
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

        // 1-bit flow direction
        uint8_t left_flow = (left >> bit_index) & 1;
        uint8_t right_flow = (right >> bit_index) & 1;
        uint8_t up_flow = (up >> (bit_index + 1)) & 1;
        uint8_t down_flow = (down >> (bit_index + 1)) & 1;

        const uint8_t red = (left_flow << 4) | (down_flow << 4);
        const uint8_t green = (right_flow << 5) | (down_flow << 5);
        const uint8_t blue = (up_flow << 4) | (down_flow << 4);
        rgb565[pixel_index] = (red << 11) | (green << 5) | blue;
      }
    }

    memcpy(prev_grad_bin, grad_bin, sizeof(grad_bin));

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
    printf("nrOfCycles: %d %d %d\n", cycles, stall, idle);
  }
}
