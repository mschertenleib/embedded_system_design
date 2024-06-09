# Embedded System Design - Optic Flow project

EPFL CS-476 Embedded System Design project by Gilles Regamey and Mathieu Schertenleib

## Goal

## Methods

![elementary_motion_detector](elementary_motion_detector.png)

## Testing

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

## Results
