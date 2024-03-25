module rgb565Grayscale (
    input  wire [15:0] rgb565,
    output wire [ 7:0] grayscale
);
  wire [15:0] swaprgb;
  assign swaprgb[15:8] = rgb565[7:0];
  assign swaprgb[7:0]  = rgb565[15:8];

  wire [4:0] red = swaprgb[15:11];
  wire [5:0] green = swaprgb[10:5];
  wire [4:0] blue = swaprgb[4:0];

  // 54 * red = (32 + 16 + 4 + 2) * red
  wire [15:0] scaled_red = (red << 5) + (red << 4) + (red << 2) + (red << 1);
  // 183 * green = (128 + 32 + 16 + 4 + 2 + 1) * green
  wire[15:0] scaled_green = (green << 7) + (green << 5) + (green << 4) + (green << 2) + (green << 1) + green;
  // 19 * blue = (16 + 2 + 1) * blue
  wire [15:0] scaled_blue = (blue << 4) + (blue << 1) + blue;

  assign grayscale = (scaled_red >> 5) + (scaled_green >> 6) + (scaled_blue >> 5);

endmodule

