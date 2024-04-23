`timescale 1ps / 1ps

module ramDmaCi_tb;

  reg s_start, clock, s_reset;
  reg [31:0] s_valueA;
  reg [31:0] s_valueB;
  reg [7:0] s_ciN;
  wire done;
  wire [31:0] result;

  initial begin
    clock = 1'b0;  /* set the initial values */
    forever #5 clock = ~clock;  /* generate a clock with a period of 10 time-units */
  end


  ramDmaCi #(
      .customId(8'd14)
  ) DUT (
      .start(s_start),
      .clock(clock),
      .reset(s_reset),
      .valueA(s_valueA),
      .valueB(s_valueB),
      .ciN(s_ciN),
      .done(done),
      .result(result)
  );

  task automatic test;
    input start;
    input [7:0] ciN;
    input [31:0] valA, valB;
    input expDone;
    input [31:0] expRes;
    begin
      s_start = start;
      s_ciN = ciN;
      s_valueA = valA;
      s_valueB = valB;
      #1;
      if ((done == expDone) && (result == expRes)) begin
        $write("\033[1;32m");
        $display(
            "[PASSED] start=%b, ciN=%0d, valueA=%0h, valueB=%0h => done=%b (exp %b), result=%0h (exp %0h)",
            s_start, s_ciN, s_valueA, s_valueB, done, expDone, result, expRes);
      end else begin
        $write("\033[1;31m");
        $display(
            "[FAILED] start=%b, ciN=%0d, valueA=%0h, valueB=%0h => done=%b (exp %b), result=%0h (exp %0h)",
            s_start, s_ciN, s_valueA, s_valueB, done, expDone, result, expRes);
      end
      $write("\033[0m");
    end
  endtask


  initial begin
    // Check that instruction only activates on start and correct ciN
    test(.start(1'b0), .ciN(8'd7), .valA(32'h0), .valB(32'h0), .expDone(1'b0), .expRes(32'h0));
    test(.start(1'b1), .ciN(8'd7), .valA(32'h0), .valB(32'h0), .expDone(1'b0), .expRes(32'h0));
    test(.start(1'b0), .ciN(8'd14), .valA(32'h0), .valB(32'h0), .expDone(1'b0), .expRes(32'h0));

    // Write then read at address 0
    test(.start(1'b1), .ciN(8'd14), .valA(32'h200), .valB(32'h42), .expDone(1'h1), .expRes(32'h0));
    @(negedge clock);
    test(.start(1'b1), .ciN(8'd14), .valA(32'h000), .valB(32'h0), .expDone(1'h0), .expRes(32'h0));
    @(negedge clock);
    test(.start(1'b1), .ciN(8'd14), .valA(32'h000), .valB(32'h0), .expDone(1'b1), .expRes(32'h42));

    // Write then read at a random address
    test(.start(1'b1), .ciN(8'd14), .valA(32'h237), .valB(32'h57), .expDone(1'b1), .expRes(32'h0));
    @(negedge clock);
    test(.start(1'b1), .ciN(8'd14), .valA(32'h037), .valB(32'h0), .expDone(1'b0), .expRes(32'h0));
    @(negedge clock);
    test(.start(1'b1), .ciN(8'd14), .valA(32'h037), .valB(32'h0), .expDone(1'b1), .expRes(32'h57));

    $finish;
  end

endmodule