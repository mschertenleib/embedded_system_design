module rgbToGrayscaleCi #(
    parameter [7:0] customInstructionId = 8'd0
) (
    input  wire        start,
    input  wire [31:0] valueA,
    valueB,
    input  wire [ 7:0] ciN,
    output wire        done,
    output wire [31:0] result
);
  wire [63:0] four_pixels_rgb565;
  assign four_pixels_rgb565[15:0]  = valueA[15:0];
  assign four_pixels_rgb565[31:16] = valueA[31:16];
  assign four_pixels_rgb565[47:32] = valueB[15:0];
  assign four_pixels_rgb565[63:48] = valueB[31:16];
  wire [31:0] four_pixels_grayscale;

  rgbToGrayscale converters[0:3] (
      .rgb565(four_pixels_rgb565),
      .grayscale(four_pixels_grayscale)
  );

  wire enable = (ciN == customInstructionId) ? start : 1'b0;
  assign done   = enable;
  assign result = enable ? four_pixels_grayscale : 32'd0;

endmodule

