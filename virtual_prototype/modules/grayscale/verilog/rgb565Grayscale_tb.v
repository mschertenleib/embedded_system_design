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
      rgb565[15:13] = green[2:0];
      rgb565[12:8]  = blue;
      rgb565[7:3]   = red;
      rgb565[2:0]   = green[5:3];
      #1
      $display(
          "rgb565=%4h={%2d,%2d,%2d} => grayscale=%3d (expected %3d)",
          rgb565,
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
    test(.red(31), .green(0), .blue(0), .expected_grayscale(54));
    test(.red(0), .green(63), .blue(0), .expected_grayscale(183));
    test(.red(0), .green(0), .blue(31), .expected_grayscale(19));
    test(.red(8), .green(16), .blue(8), .expected_grayscale(65));
    test(.red(16), .green(32), .blue(16), .expected_grayscale(130));
    test(.red(24), .green(48), .blue(24), .expected_grayscale(195));

    $finish;
  end

endmodule
