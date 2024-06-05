#include <opencv2/opencv.hpp>

#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>

#define WIDTH 640
#define HEIGHT 480

int main() {
  cv::VideoCapture cap(0);
  cap.set(cv::CAP_PROP_FRAME_WIDTH, WIDTH);
  cap.set(cv::CAP_PROP_FRAME_HEIGHT, HEIGHT);

  if (!cap.isOpened()) {
    std::cout << "Failed to open capture" << std::endl;
    return -1;
  }

  cv::namedWindow("img", cv::WINDOW_NORMAL);

  const auto target_frame_time = std::chrono::duration<float>{1.0f / 15.0f};
  auto last_time = std::chrono::steady_clock::now();

  cv::Mat grayscale_mat;
  cv::Mat rgb565_mat;

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
    last_time += target_frame_time;

    cv::Mat frame;
    bool ret = cap.read(frame);
    if (!ret) {
      std::cout << "Cannot read frame\n";
      break;
    }

    cv::cvtColor(frame, grayscale_mat, cv::COLOR_RGB2GRAY);

    const auto *const grayscale =
        static_cast<const uint8_t *>(grayscale_mat.data);
    auto *const rgb565 = static_cast<uint16_t *>(rgb565_mat.data);

    // ------------ C code BEGIN -------------------------

    rgb565[4000] = 0xffff;

    // ------------ C code END ---------------------------

    cv::imshow("img", rgb565_mat);
    if (cv::waitKey(1) == 27) {
      break;
    }
  }

  cap.release();
  cv::destroyAllWindows();
}
