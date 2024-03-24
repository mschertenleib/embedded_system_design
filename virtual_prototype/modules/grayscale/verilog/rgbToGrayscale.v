module rgbToGrayscale (
    input  wire [15:0] rgb565,
    output wire [ 7:0] grayscale
);

  wire [4:0] red;
  wire [5:0] green;
  wire [4:0] blue;
  assign red   = rgb565[15:11];
  assign green = rgb565[10:5];
  assign blue  = rgb565[4:0];

  wire [10:0] scaled_red;
  wire [13:0] scaled_green;
  wire [ 9:0] scaled_blue;
  // 54 * red = (32 + 16 + 4 + 2) * red
  assign scaled_red = (red << 5) + (red << 4) + (red << 2) + (red << 1);
  // 183 * green = (128 + 32 + 16 + 4 + 2 + 1) * green
  assign scaled_green = (green << 7) + (green << 5) + (green << 4) + (green << 2) + (green << 1) + green;
  // 19 * blue = (16 + 2 + 1) * blue
  assign scaled_blue = (blue << 4) + (blue << 1) + blue;

  wire [15:0] scaled_sum;
  assign scaled_sum = red_scaled + scaled_green + scaled_blue;
  assign grayscale  = scaled_sum >> 8;

endmodule

