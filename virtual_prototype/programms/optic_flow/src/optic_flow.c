#include <ov7670.h>
#include <stdio.h>
#include <swap.h>
#include <vga.h>

#define GRAD_THRESHOLD 30

int main() {
  const uint8_t sevenSeg[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66,
                                0x6D, 0x7D, 0x07, 0x7F, 0x6F};

  // Output image (color shows flow)
  volatile uint16_t rgb565[640 * 480];
  // Input image
  volatile uint8_t grayscale[640 * 480];
  // Binary gradients
  volatile uint32_t grad_bin_x[640 / 32 * 480];
  volatile uint32_t prev_grad_bin_x[640 / 32 * 480];
  volatile uint32_t grad_bin_y[640 / 32 * 480];
  volatile uint32_t prev_grad_bin_y[640 / 32 * 480];

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
    for (int base_pixel = camParams.nrOfPixelsPerLine;
         base_pixel < camParams.nrOfLinesPerImage - camParams.nrOfPixelsPerLine;
         base_pixel += camParams.nrOfPixelsPerLine) {
      for (int j = 1; j < camParams.nrOfPixelsPerLine - 1; ++j) {

        const int pixel_index = base_pixel + j;
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

        const int base_bin_index = pixel_index >> 5;
        const int bit_index = pixel_index & 31;
        grad_bin_x[base_bin_index] |= dx << bit_index;
        grad_bin_y[base_bin_index] |= dy << bit_index;
      }
    }

    // Convert binary gradients to optic flow
    for (int base_pixel = camParams.nrOfPixelsPerLine;
         base_pixel < camParams.nrOfLinesPerImage - camParams.nrOfPixelsPerLine;
         base_pixel += camParams.nrOfPixelsPerLine) {
      for (int j = 1; j < camParams.nrOfPixelsPerLine - 1; ++j) {

        const int pixel_index = base_pixel + j;
        const int base_bin_index = pixel_index >> 5;
        const int bit_index = pixel_index & 31;
        const uint8_t grad_x = (grad_bin_x[base_bin_index] >> bit_index) & 1;
        const uint8_t grad_y = (grad_bin_y[base_bin_index] >> bit_index) & 1;
        rgb565[pixel_index] =
            ((uint16_t)grad_x << 15) | ((uint16_t)grad_y << 4);
      }
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
    printf("nrOfCycles: %d %d %d\n", cycles, stall, idle);
  }
}
