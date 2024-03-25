`timescale 1ps / 1ps

module rgb565Grayscale_tb;

  reg  [15:0] rgb565;
  wire [ 7:0] grayscale;

  rgb565Grayscale dut (
      .rgb565(rgb565),
      .grayscale(grayscale)
  );

  task automatic test;
    input [4:0] red;
    input [5:0] green;
    input [4:0] blue;
    input [7:0] expected_grayscale;
    begin
      rgb565[15:11] = red;
      rgb565[10:5]  = green;
      rgb565[4:0]   = blue;
      #1
      $display(
          "%s: rgb565={%0d,%0d,%0d} => grayscale=%0d (expected %0d)",
          (grayscale == expected_grayscale) ? "Passed" : "Failed",
          red,
          green,
          blue,
          grayscale,
          expected_grayscale
      );
    end
  endtask

  initial begin
    test(.red(0), .green(0), .blue(0), .expected_grayscale(0));
    test(.red(31), .green(63), .blue(31), .expected_grayscale(255));

    $finish;
  end

endmodule
