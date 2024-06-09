#include <opencv2/opencv.hpp>

#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <thread>

#define WIDTH 640
#define HEIGHT 480
#define GRAD_THRESHOLD 20

int main()
{
  constexpr struct camParameters
  {
    int nrOfPixelsPerLine;
    int nrOfLinesPerImage;
  } camParams{.nrOfPixelsPerLine = WIDTH, .nrOfLinesPerImage = HEIGHT};

  cv::VideoCapture cap(0);
  cap.set(cv::CAP_PROP_FRAME_WIDTH, WIDTH);
  cap.set(cv::CAP_PROP_FRAME_HEIGHT, HEIGHT);

  if (!cap.isOpened())
  {
    std::cout << "Failed to open capture" << std::endl;
    return -1;
  }

  const auto target_frame_time = std::chrono::duration<float>{1.0f / 15.0f};
  auto last_time = std::chrono::steady_clock::now();

  cv::Mat grayscale_mat(cv::Size(WIDTH, HEIGHT), CV_8U);
  cv::Mat rgb_mat(cv::Size(WIDTH, HEIGHT), CV_8UC3);

  std::uint32_t grad_buffers[2][WIDTH / 32 * HEIGHT * 2]{};
  int current_buffer = 0;

  while (cap.isOpened())
  {
    const auto current_time = std::chrono::steady_clock::now();
    const auto elapsed_time = current_time - last_time;
    if (elapsed_time < target_frame_time)
    {
      std::this_thread::sleep_for(target_frame_time - elapsed_time);
    }
    last_time +=
        std::chrono::duration_cast<std::chrono::steady_clock::duration>(
            target_frame_time);

    cv::Mat frame;
    bool ret = cap.read(frame);
    if (!ret)
    {
      std::cout << "Cannot read frame\n";
      break;
    }

    cv::cvtColor(frame, grayscale_mat, cv::COLOR_RGB2GRAY);

    const auto *const grayscale =
        reinterpret_cast<const uint8_t *>(grayscale_mat.data);

    // ------------ C code BEGIN -------------------------

    // Convert grayscale to binary gradients

    uint32_t *grad_bin = grad_buffers[current_buffer];
    uint32_t *prev_grad_bin = grad_buffers[1 - current_buffer];

    // Skip first and last rows and cols
    for (int pixel_index = camParams.nrOfPixelsPerLine;
         pixel_index <
         (camParams.nrOfLinesPerImage - 1) * camParams.nrOfPixelsPerLine;
         ++pixel_index)
    {

      const uint8_t gray_left = grayscale[pixel_index - 1];
      const uint8_t gray_right = grayscale[pixel_index + 1];
      const uint8_t gray_up =
          grayscale[pixel_index - camParams.nrOfPixelsPerLine];
      const uint8_t gray_down =
          grayscale[pixel_index + camParams.nrOfPixelsPerLine];

      uint8_t dx;
      if (gray_right >= gray_left)
      {
        dx = gray_right - gray_left > GRAD_THRESHOLD;
      }
      else
      {
        dx = gray_left - gray_right > GRAD_THRESHOLD;
      }
      uint8_t dy;
      if (gray_up >= gray_down)
      {
        dy = gray_up - gray_down > GRAD_THRESHOLD;
      }
      else
      {
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
         ++base_index)
    {

      const int base_index_next_row =
          base_index + (camParams.nrOfPixelsPerLine >> 4);

      const uint32_t left_and =
          grad_bin[base_index] & (prev_grad_bin[base_index] >> 2);
      const uint32_t right_and =
          (grad_bin[base_index] >> 2) & prev_grad_bin[base_index];
      const uint32_t left = left_and & ~right_and;
      const uint32_t right = right_and & ~left_and;

      const uint32_t up_and =
          grad_bin[base_index] & prev_grad_bin[base_index_next_row];
      const uint32_t down_and =
          grad_bin[base_index_next_row] & prev_grad_bin[base_index];
      const uint32_t up = up_and & ~down_and;
      const uint32_t down = down_and & ~up_and;

      for (int j = 0; j < 16; ++j)
      {
        const int pixel_index = (base_index << 4) + j;
        const int bit_index = (pixel_index & 15) << 1;

        // 1-bit flow direction
        uint8_t left_flow = (left >> bit_index) & 1;
        uint8_t right_flow = (right >> bit_index) & 1;
        uint8_t up_flow = (up >> (bit_index + 1)) & 1;
        uint8_t down_flow = (down >> (bit_index + 1)) & 1;

        rgb_mat.data[pixel_index * 3 + 0] = (up_flow << 7) | (down_flow << 7);
        rgb_mat.data[pixel_index * 3 + 1] =
            (right_flow << 7) | (down_flow << 7);
        rgb_mat.data[pixel_index * 3 + 2] = (left_flow << 7) | (down_flow << 7);
      }
    }

    current_buffer = 1 - current_buffer;

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
