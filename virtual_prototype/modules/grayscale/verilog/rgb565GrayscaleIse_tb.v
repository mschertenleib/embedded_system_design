`timescale 1ps / 1ps

module rgb565GrayscaleIse_tb;

  reg start;
  reg [7:0] ciN;
  reg [31:0] valueA;
  reg [31:0] valueB;
  wire done;
  wire [31:0] result;

  rgb565GrayscaleIse #(
      .customInstructionId(8'd13)
  ) dut (
      .start (start),
      .valueA(valueA),
      .valueB(valueB),
      .iseId (ciN),
      .done  (done),
      .result(result)
  );

  task automatic test_ci;
    input test_start;
    input [7:0] test_ciN;
    input [31:0] test_valueA, test_valueB;
    input expected_done;
    input [31:0] expected_result;
    begin
      start = test_start;
      valueA = test_valueA;
      valueB = test_valueB;
      ciN = test_ciN;
      #1
      $display(
          "start=%b, ciN=%0d, valueA=%0h, valueB=%0h => done=%b (expected %b), result=%0h (expected %0h)",
          start,
          ciN,
          valueA,
          valueB,
          done,
          expected_done,
          result,
          expected_result
      );
    end
  endtask

  function automatic [15:0] rgb;
    input [4:0] r;
    input [5:0] g;
    input [4:0] b;
    begin
      rgb[15:11] = r;
      rgb[10:5]  = g;
      rgb[4:0]   = b;
    end
  endfunction

  task automatic test_4way;
    input [15:0] pixel0, pixel1, pixel2, pixel3;
    begin
      start = 1'b1;
      ciN = 8'd13;
      valueA[15:0] = pixel0;
      valueA[31:16] = pixel1;
      valueB[15:0] = pixel2;
      valueB[31:16] = pixel3;
      #1
      $display(
          "%4h %4h %4h %4h => %2h %2h %2h %2h",
          pixel0,
          pixel1,
          pixel2,
          pixel3,
          result[7:0],
          result[15:8],
          result[23:16],
          result[31:24]
      );
    end
  endtask

  initial begin
    test_ci(.test_start(1'b0), .test_ciN(8'd47), .test_valueA(32'd0), .test_valueB(32'd0),
            .expected_done(1'b0), .expected_result(32'd0));
    test_ci(.test_start(1'b1), .test_ciN(8'd47), .test_valueA(32'd0), .test_valueB(32'd0),
            .expected_done(1'b0), .expected_result(32'd0));
    test_ci(.test_start(1'b0), .test_ciN(8'd13), .test_valueA(32'd0), .test_valueB(32'd0),
            .expected_done(1'b0), .expected_result(32'd0));
    test_ci(.test_start(1'b1), .test_ciN(8'd13), .test_valueA(32'd0), .test_valueB(32'd0),
            .expected_done(1'b1), .expected_result(32'd0));

    test_4way(0, 0, 0, 0);
    test_4way(16'hffff, 16'hffff, 16'hffff, 16'hffff);
    test_4way(rgb(8, 16, 8), rgb(16, 32, 16), rgb(24, 48, 24), rgb(31, 63, 31));

    test_ci(.test_start(1'b1), .test_ciN(8'd13), .test_valueA(32'h84104208),
            .test_valueB(32'hffffc618), .expected_done(1'b1), .expected_result(32'hfabf7f3e));
    test_ci(.test_start(1'b1), .test_ciN(8'd47), .test_valueA(32'h84104208),
            .test_valueB(32'hffffc618), .expected_done(1'b0), .expected_result(32'h0));

    $finish;
  end

endmodule
