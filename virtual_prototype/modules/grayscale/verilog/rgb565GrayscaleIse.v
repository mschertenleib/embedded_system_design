module rgb565GrayscaleIse #(
    parameter [7:0] customInstructionId = 8'd0
) (
    input  wire        start,
    input  wire [31:0] valueA,
    valueB,
    input  wire [ 7:0] iseId,
    output wire        done,
    output wire [31:0] result
);
  wire [63:0] four_pixels_rgb565;
  assign four_pixels_rgb565[15:0]  = valueA[15:0];
  assign four_pixels_rgb565[31:16] = valueA[31:16];
  assign four_pixels_rgb565[47:32] = valueB[15:0];
  assign four_pixels_rgb565[63:48] = valueB[31:16];
  wire [31:0] four_pixels_grayscale;

  rgb565Grayscale converters[0:3] (
      .rgb565(four_pixels_rgb565),
      .grayscale(four_pixels_grayscale)
  );

  /*
  assign result[7:0]   = four_pixels_grayscale[7:0];
  assign result[15:8]  = four_pixels_grayscale[15:8];
  assign result[23:16] = four_pixels_grayscale[23:16];
  assign result[31:24] = four_pixels_grayscale[31:24];
  */

  wire enable = (iseId == customInstructionId) ? start : 1'b0;
  assign done   = enable;
  assign result = enable ? four_pixels_grayscale : 32'd0;

endmodule

