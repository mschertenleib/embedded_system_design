# Embedded System Design - Optic Flow project

EPFL CS-476 Embedded System Design project by Gilles Regamey and Mathieu Schertenleib

## Goal

Optic flow was first used to describe the visual stimulus provided to animals moving through the world. It is usually used for optical mouses and visual odometry.

For a pixel at location (x,y,t) with intensity I(x,y,t) will have moved by Δx, Δy and Δt between two image frames

Assuming the movement to be small, the image constraint at I(x,y,t) linearized is

Need for an additional constraint to solve for optimal Δx, Δy , Usually by maximizing cross correlation or minimizing image differences.

**Explored solutions:**

- Gradient methods: out of reach
- Image Interpolation: requires compromises (needs multiplications and divisions, quite high memory usage)
- Elementary Motion Detector: biologically inspired, analog
- Binary Elementary Motion Detector: efficient, fast, 1 pixel

## Methods

The selected solution is to use a binary elementary motion detector (see image below). It is efficient, fast, and only requires a single bit of memory per pixel. Its biggest drawback is its relatively high sensitivity to noise compared to other techniques, as well as only being able to compute single pixel deltas per frame. Additionally, it is dependent on framerate.

To be able to detect objects moving faster than one pixel per frame, we anticipated to add additional elementary cells computing deltas between pixels further apart. Due to a high complexity of implementation and greatly increased memory transfer requirements leading to slower framerates, we decided to abandon this extra functionality.

In order to increase the robustness of the detection, we first preprocess the image by transforming it into grayscale, the performing a binary edge detection using thresholded intensity gradients.

![elementary_motion_detector](elementary_motion_detector.png)

### Testing

In order to validate our method, we developed a [Python program](tests/tests.py), then a [C++ program](tests/tests.cpp). These use the OpenCV library to access the webcam and offer a quick testing cycle for validating that the proposed algorithm could work. Additionally, the C++ program is setup such as to be able to directly copy-paste the pure C code that would be compiled for the virtual prototype. It is built using [CMake](tests/CMakeLists.txt):

```bash
cd tests
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
./build/tests
```

The program opens the webcam, and displays the binary optic flow using the following color code:

- Left: red
- Right: green
- Up: blue
- Down: white

![flow_cpp](flow_cpp.png)

### Streaming

Ideally, we would implement a buffer with one bit per pixel directly in the camera module, allowing us to compute the whole RGB565 to optic flow computation in a streaming fashion, and therefore achieving a constant 15 fps (or even 30 by changing camera parameters). However, the limited on-chip FPGA memory means we have to store the gradient buffer in the external SRAM. Therefore, we compute the binary (dx, dy) gradients in the camera module, and transfer whole rows to the external buffer.

TODO: diagramme de Gilles qui est sur la présentation

The conversion from binary gradients to optic flow and then colors for visualization is done on the CPU. We test different implementations: using a pure C implementation, using custom instructions for conversions, as well as using DMA for memory transfers.
We do the same tests for optionally doing the grayscale to binary gradient conversion on the CPu rather than in a streaming fashion.

## Results

We obtain the following performance results:

### 8-bit grayscale to binary gradient

|                 | Cycles     | Stall cycles | Bus idle cycles |
|-----------------|------------|--------------|-----------------|
| Pure C          | 87'074'811 | 67'126'746   | 35'911'724      |
| With DMA        | 20'117'809 | 399'151      | 13'998'973      |
| With DMA and CI | 14'924'915 | 358'056      | 10'247'412      |
| Streaming       | 0          | 0            | 0               |

### Binary gradient to binary flow direction

|                         | Cycles     | Stall cycles | Bus idle cycles |
|-------------------------|------------|--------------|-----------------|
| Pure C                  | 25'640'805 | 14'374'713   | 14'398'432      |
| Optic flow CI, no DMA   | 10'447'509 | 7'918'379    | 4'530'602       |
| Optic flow CI, with DMA | 4'898'731  | 733'720      | 1'751'847       |

TODO: capture d'écran de l'image qu'on obtient + photo de ce qu'on filme
