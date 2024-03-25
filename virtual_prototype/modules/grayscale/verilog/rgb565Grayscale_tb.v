`timescale 1ps / 1ps

module rgb565Grayscale_tb;

  reg  [15:0] rgb565;
  wire [ 7:0] grayscale;

  rgb565Grayscale dut (
      .rgb565(rgb565),
      .grayscale(grayscale)
  );

  task automatic test;
    input [15:0] test_rgb565;
    input [7:0] expected_grayscale;

    begin
      rgb565 = test_rgb565;
      #1
      $display(
          "%s: rgb565=%0d => grayscale=%0d (expected %0d)",
          (grayscale == expected_grayscale) ? "Passed" : "Failed",
          rgb565,
          grayscale,
          expected_grayscale
      );
    end
  endtask

  initial begin
    test(.test_rgb565(16'h0), .expected_grayscale(8'h0));
    test(.test_rgb565(16'hffff), .expected_grayscale(8'hff));

    $finish;
  end

endmodule
