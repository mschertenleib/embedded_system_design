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
    #ifdef UNOPT
    for (int line = 0; line < camParams.nrOfLinesPerImage; line++) {
      for (int pixel = 0; pixel < camParams.nrOfPixelsPerLine; pixel++) {
        uint16_t rgb = swap_u16(rgb565[line * camParams.nrOfPixelsPerLine + pixel]);
        uint32_t red1 = ((rgb >> 11) & 0x1F) << 3;
        uint32_t green1 = ((rgb >> 5) & 0x3F) << 2;
        uint32_t blue1 = (rgb & 0x1F) << 3;
        uint32_t gray = ((red1 * 54 + green1 * 183 + blue1 * 19) >> 8) & 0xFF;
        grayscale[line * camParams.nrOfPixelsPerLine + pixel] = gray;
      }
    }
    #elif defined(SINGLEPX)
        for (int px = 0; px < camParams.nrOfLinesPerImage * camParams.nrOfPixelsPerLine; ++px) {
          grayscale[px] = rgb2gray(rgb565[px]);
        }
    #elif defined(PARALLEL)
        for (int px = 0;
            px < camParams.nrOfLinesPerImage * camParams.nrOfPixelsPerLine;
            px += 4) {
          rgb2gray_parallel(&grayscale[px], &rgb565[px]);
        }
    #endif

    control_counters(DISABLE_CYCLES | DISABLE_BUS_IDLE | DISABLE_STALL | DISABLE_CYCLES_2);
    stall = read_counter(COUNTER_STALL);
    idle = read_counter(COUNTER_BUS_IDLE);
    cycles = read_counter(COUNTER_CYCLES);
    printf("C: %lu S: %lu BI: %lu Eff: %lu\n", cycles, stall, idle, cycles - stall);
  }
}