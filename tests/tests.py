import numpy as np
import time
import cv2

WIDTH = 640
HEIGHT = 360


def spatial_corr_x(
    current: np.ndarray, delayed: np.ndarray, distance: int
) -> tuple[np.ndarray, np.ndarray]:
    left = current[:, :-distance] & delayed[:, distance:]
    right = current[:, distance:] & delayed[:, :-distance]
    dx = np.zeros_like(current, dtype=np.int8)

    dx[:, distance // 2 : -(distance - distance // 2)] = right.astype(
        np.int8
    ) - left.astype(np.int8)
    return dx


def spatial_corr_y(
    current: np.ndarray, delayed: np.ndarray, distance: int
) -> tuple[np.ndarray, np.ndarray]:
    up = current[:-distance, :] & delayed[distance:, :]
    down = current[distance:, :] & delayed[:-distance, :]
    dy = np.zeros_like(current, dtype=np.int8)

    dy[distance // 2 : -(distance - distance // 2), :] = down.astype(
        np.int8
    ) - up.astype(np.int8)
    return dy


def as_color(
    corr_x: np.ndarray,
    corr_y: np.ndarray,
    scale: int,
    draw_arrow: bool = False,
    mean_size: int = 11,
    arrow_scale: float = 50.0,
) -> np.ndarray:
    img = np.zeros((HEIGHT, WIDTH, 3), dtype=np.uint8)
    img[:, :, 1] = np.maximum(corr_x, 0) * scale
    img[:, :, 2] = np.maximum(-corr_x, 0) * scale

    if draw_arrow:
        for y in range(mean_size // 2, HEIGHT, mean_size):
            for x in range(mean_size // 2, WIDTH, mean_size):
                cv2.arrowedLine(
                    img,
                    pt1=(x, y),
                    pt2=(
                        x
                        + int(
                            np.mean(
                                corr_x[
                                    y
                                    - mean_size // 2 : y
                                    + (mean_size - mean_size // 2),
                                    x
                                    - mean_size // 2 : x
                                    + (mean_size - mean_size // 2),
                                ]
                            )
                            * scale
                            / 255.0
                            * arrow_scale
                        ),
                        y
                        + int(
                            np.mean(
                                corr_y[
                                    y
                                    - mean_size // 2 : y
                                    + (mean_size - mean_size // 2),
                                    x
                                    - mean_size // 2 : x
                                    + (mean_size - mean_size // 2),
                                ]
                            )
                            * scale
                            / 255.0
                            * arrow_scale
                        ),
                    ),
                    color=(255, 255, 255),
                )

    return img


def edges_x(img: np.ndarray) -> np.ndarray:
    left = np.clip(img[:, :-2], 0, 127).astype(np.int8)
    right = np.clip(img[:, 2:], 0, 127).astype(np.int8)
    grad_x = np.zeros_like(img)
    grad_x[:, 1:-1] = np.abs(right - left)
    grad_x = np.clip(grad_x, 0, 127).astype(np.uint8) * 2
    return grad_x


def edges_y(img: np.ndarray) -> np.ndarray:
    up = np.clip(img[:-2, :], 0, 127).astype(np.int8)
    down = np.clip(img[2:, :], 0, 127).astype(np.int8)
    grad_y = np.zeros_like(img)
    grad_y[1:-1, :] = np.abs(down - up)
    grad_y = np.clip(grad_y, 0, 127).astype(np.uint8) * 2
    return grad_y


def main():
    cap = cv2.VideoCapture(0)
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, WIDTH)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, HEIGHT)
    if not cap.isOpened():
        print("Failed to open capture")
        return

    cv2.namedWindow("img", cv2.WINDOW_NORMAL)

    prev_x = np.zeros((HEIGHT, WIDTH), dtype=np.uint8)
    prev_y = np.zeros((HEIGHT, WIDTH), dtype=np.uint8)
    img = np.zeros((2 * HEIGHT, 2 * WIDTH, 3), dtype=np.uint8)

    target_fps = 15
    target_frame_time = 1.0 / target_fps
    last_time = time.time()

    while cap.isOpened():
        elapsed_time = time.time() - last_time
        if elapsed_time < target_frame_time:
            time.sleep(target_frame_time - elapsed_time)
        last_time += target_frame_time

        ret, frame = cap.read()
        if not ret:
            print("Cannot read frame")
            break

        grayscale = cv2.cvtColor(frame, cv2.COLOR_RGB2GRAY)
        grad_x = edges_x(grayscale)
        grad_y = edges_y(grayscale)
        _, grad_x_bin = cv2.threshold(
            grad_x, thresh=30, maxval=1, type=cv2.THRESH_BINARY
        )
        _, grad_y_bin = cv2.threshold(
            grad_y, thresh=30, maxval=1, type=cv2.THRESH_BINARY
        )

        total_corr_x = np.zeros((HEIGHT, WIDTH), dtype=np.int8)
        total_corr_y = np.zeros((HEIGHT, WIDTH), dtype=np.int8)
        for i in range(1, 16):
            corr_x = spatial_corr_x(grad_x_bin, prev_x, i)
            corr_y = spatial_corr_y(grad_y_bin, prev_y, i)
            total_corr_x = np.where(corr_x != 0, corr_x * i, total_corr_x)
            total_corr_y = np.where(corr_y != 0, corr_y * i, total_corr_y)

        corr_x = spatial_corr_x(grad_x_bin, prev_x, 1)
        corr_y = spatial_corr_y(grad_y_bin, prev_y, 1)

        prev_x[:] = grad_x_bin
        prev_y[:] = grad_y_bin

        img[:HEIGHT, :WIDTH, :] = frame
        img[:HEIGHT, WIDTH:, 2] = grad_x
        img[:HEIGHT, WIDTH:, 1] = grad_y
        img[HEIGHT:, :WIDTH, :] = as_color(corr_x, corr_y, scale=255)
        img[HEIGHT:, WIDTH:, :] = as_color(
            total_corr_x,
            total_corr_y,
            scale=15,
            draw_arrow=True,
            mean_size=20,
            arrow_scale=50.0,
        )

        cv2.imshow("img", img)
        if cv2.waitKey(1) & 0xFF == 27:
            break

    cap.release()
    cv2.destroyAllWindows()


if __name__ == "__main__":
    main()
