/* set the time-units for simulation */
`timescale 1ns / 1ps

module rgb565Grayscale_tb;

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

  task automatic test_case;
    input test_start;
    input [7:0] test_ciN;
    input [31:0] test_valueA, test_valueB;
    input expected_done;
    input expected_result;

    begin
      start = test_start;
      valueA = test_valueA;
      valueB = test_valueB;
      ciN = test_ciN;
      #1
      $display(
          "%s: start=%b, ciN=%0d, valueA=%0d, valueB=%0d => done=%b (expected %b), result=%0d (expected %0d)",
          ((done == expected_done) && (result == expected_result)) ? "Passed" : "Failed",
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

  initial begin
    $dumpfile("rgb565Grayscale_signals.vcd");
    $dumpvars(1, dut);
  end

  initial begin
    // Check correct response to start/ciN
    test_case(.test_start(1'b0), .test_ciN(8'd47), .test_valueA(32'd0), .test_valueB(32'd0),
              .expected_done(1'b0), .expected_result(32'd0));
    test_case(.test_start(1'b1), .test_ciN(8'd47), .test_valueA(32'd0), .test_valueB(32'd0),
              .expected_done(1'b0), .expected_result(32'd0));
    test_case(.test_start(1'b0), .test_ciN(8'd13), .test_valueA(32'd0), .test_valueB(32'd0),
              .expected_done(1'b0), .expected_result(32'd0));
    test_case(.test_start(1'b1), .test_ciN(8'd13), .test_valueA(32'd0), .test_valueB(32'd0),
              .expected_done(1'b1), .expected_result(32'd0));

    // Test conversions
    test_case(.test_start(1'b0), .test_ciN(8'd13), .test_valueA(32'd0), .test_valueB(32'd0),
              .expected_done(1'b0), .expected_result(32'd0));
    test_case(.test_start(1'b0), .test_ciN(8'd13), .test_valueA(32'd0), .test_valueB(32'd0),
              .expected_done(1'b0), .expected_result(32'd0));

    $finish;
  end

endmodule
