#include <opencv2/opencv.hpp>

#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>

#define WIDTH 640
#define HEIGHT 480
#define GRAD_THRESHOLD 20

int main() {
  constexpr struct camParameters {
    int nrOfPixelsPerLine;
    int nrOfLinesPerImage;
  } camParams{.nrOfPixelsPerLine = WIDTH, .nrOfLinesPerImage = HEIGHT};

  cv::VideoCapture cap(0);
  cap.set(cv::CAP_PROP_FRAME_WIDTH, WIDTH);
  cap.set(cv::CAP_PROP_FRAME_HEIGHT, HEIGHT);

  if (!cap.isOpened()) {
    std::cout << "Failed to open capture" << std::endl;
    return -1;
  }

  const auto target_frame_time = std::chrono::duration<float>{1.0f / 15.0f};
  auto last_time = std::chrono::steady_clock::now();

  cv::Mat grayscale_mat(cv::Size(WIDTH, HEIGHT), CV_8U);
  cv::Mat rgb_mat(cv::Size(WIDTH, HEIGHT), CV_8UC3);

  std::uint32_t grad_bin_x[WIDTH / 32 * HEIGHT]{};
  std::uint32_t prev_grad_bin_x[WIDTH / 32 * HEIGHT]{};
  std::uint32_t grad_bin_y[WIDTH / 32 * HEIGHT]{};
  std::uint32_t prev_grad_bin_y[WIDTH / 32 * HEIGHT]{};

  while (cap.isOpened()) {
    const auto current_time = std::chrono::steady_clock::now();
    const auto elapsed_time = current_time - last_time;
    if (elapsed_time < target_frame_time) {
      std::this_thread::sleep_for(target_frame_time - elapsed_time);
    }
    last_time +=
        std::chrono::duration_cast<std::chrono::steady_clock::duration>(
            target_frame_time);

    cv::Mat frame;
    bool ret = cap.read(frame);
    if (!ret) {
      std::cout << "Cannot read frame\n";
      break;
    }

    cv::cvtColor(frame, grayscale_mat, cv::COLOR_RGB2GRAY);

    const auto *const grayscale =
        reinterpret_cast<const uint8_t *>(grayscale_mat.data);

    // ------------ C code BEGIN -------------------------

    // Convert grayscale to binary gradients
    for (int base_pixel = camParams.nrOfPixelsPerLine;
         base_pixel <
         (camParams.nrOfLinesPerImage - 1) * camParams.nrOfPixelsPerLine;
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
        grad_bin_x[base_bin_index] =
            (grad_bin_x[base_bin_index] & ~(1 << bit_index)) |
            (dx << bit_index);
        grad_bin_y[base_bin_index] =
            (grad_bin_y[base_bin_index] & ~(1 << bit_index)) |
            (dy << bit_index);
      }
    }

    // Convert binary gradients to optic flow
    for (int base_pixel = camParams.nrOfPixelsPerLine;
         base_pixel <
         (camParams.nrOfLinesPerImage - 1) * camParams.nrOfPixelsPerLine;
         base_pixel += camParams.nrOfPixelsPerLine) {
      for (int j = 1; j < camParams.nrOfPixelsPerLine - 1; ++j) {

        const int pixel_index = base_pixel + j;
        const int base_bin_index = pixel_index >> 5;
        const int bit_index = pixel_index & 31;
        const uint8_t grad_x = (grad_bin_x[base_bin_index] >> bit_index) & 1;
        const uint8_t grad_y = (grad_bin_y[base_bin_index] >> bit_index) & 1;
        rgb_mat.data[pixel_index * 3 + 0] = grad_x << 7;
        rgb_mat.data[pixel_index * 3 + 1] = grad_y << 7;
        rgb_mat.data[pixel_index * 3 + 2] = 0;
      }
    }

    // ------------ C code END ---------------------------

    cv::imwrite("img.png", rgb_mat);
    // cv::imshow("img", rgb565_mat);
    // if (cv::waitKey(1) == 27) {
    //  break;
    //}
  }

  cap.release();
  // cv::destroyAllWindows();
}
