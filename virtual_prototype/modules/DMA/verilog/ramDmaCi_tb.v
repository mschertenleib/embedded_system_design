`timescale 1ps / 1ps

module ramDmaCi_tb;

  reg s_start = 1'b0, clock = 1'b0, s_reset = 1'b0;
  reg [31:0] s_valueA = 32'b0;
  reg [31:0] s_valueB = 32'b0;
  reg [7:0] s_ciN = 8'b0;
  wire done;
  wire [31:0] result;

  reg granted = 1'b0;
  reg [31:0] address_data_in = 32'b0;
  reg end_transaction_in = 1'b0, data_valid_in = 1'b0, busy_in = 1'b0, error_in = 1'b0;
  wire request;
  wire [31:0] address_data_out;
  wire [3:0] byte_enables_out;
  wire [7:0] burst_size_out;
  wire read_n_write_out, begin_transaction_out, end_transaction_out, data_valid_out;

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
      .result(result),
      .granted(granted),
      .address_data_in(address_data_in),
      .end_transaction_in(end_transaction_in),
      .data_valid_in(data_valid_in),
      .busy_in(busy_in),
      .error_in(error_in),
      .request(request),
      .address_data_out(address_data_out),
      .byte_enables_out(byte_enables_out),
      .burst_size_out(burst_size_out),
      .read_n_write_out(read_n_write_out),
      .begin_transaction_out(begin_transaction_out),
      .end_transaction_out(end_transaction_out),
      .data_valid_out(data_valid_out)
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
            "[PASSED] start=%b, ciN=%0d, valueA=0x%0h, valueB=0x%0h => done=%b (exp. %b), result=0x%0h (exp. 0x%0h)",
            s_start, s_ciN, s_valueA, s_valueB, done, expDone, result, expRes);
      end else begin
        $write("\033[1;31m");
        $display(
            "[FAILED] start=%b, ciN=%0d, valueA=0x%0h, valueB=0x%0h => done=%b (exp. %b), result=0x%0h (exp. 0x%0h)",
            s_start, s_ciN, s_valueA, s_valueB, done, expDone, result, expRes);
      end
      $write("\033[0m");
    end
  endtask

  initial begin
    $dumpfile("dma_signals.vcd");
    $dumpvars(1, DUT);
  end

  initial begin
    // Check that the instruction only activates on start and correct ciN
    $display("Activation");
    test(.start(1'b0), .ciN(8'd7), .valA(32'h0), .valB(32'h0), .expDone(1'b0), .expRes(32'h0));
    test(.start(1'b1), .ciN(8'd7), .valA(32'h0), .valB(32'h0), .expDone(1'b0), .expRes(32'h0));
    test(.start(1'b0), .ciN(8'd14), .valA(32'h0), .valB(32'h0), .expDone(1'b0), .expRes(32'h0));

    // Write then read at address 0
    $display("Write/read address 0");
    test(.start(1'b1), .ciN(8'd14), .valA(32'h200), .valB(32'h42), .expDone(1'h1), .expRes(32'h0));
    @(negedge clock);
    test(.start(1'b1), .ciN(8'd14), .valA(32'h000), .valB(32'h0), .expDone(1'h0), .expRes(32'h0));
    @(negedge clock);
    test(.start(1'b0), .ciN(8'd0), .valA(32'h000), .valB(32'h0), .expDone(1'b1), .expRes(32'h42));
    @(negedge clock);

    // Write then read at a random address
    $display("Write/read address 0x37");
    test(.start(1'b1), .ciN(8'd14), .valA(32'h237), .valB(32'h57), .expDone(1'b1), .expRes(32'h0));
    @(negedge clock);
    test(.start(1'b1), .ciN(8'd14), .valA(32'h037), .valB(32'h0), .expDone(1'b0), .expRes(32'h0));
    @(negedge clock);
    test(.start(1'b0), .ciN(8'd0), .valA(32'h037), .valB(32'h0), .expDone(1'b1), .expRes(32'h57));
    @(negedge clock);

    // Test bus start address
    $display("Bus start address");
    test(1'b1, 8'd14, 4'b0011 << 9, 32'h29, 1'b1, 32'h0);
    @(negedge clock);
    test(1'b1, 8'd14, 4'b0010 << 9, 32'h0, 1'b1, 32'h29);
    @(negedge clock);
    test(1'b1, 8'd14, 4'b0011 << 9, 32'h0, 1'b1, 32'h0);
    @(negedge clock);

    // Test memory start address
    $display("Memory start address");
    test(1'b1, 8'd14, 4'b0101 << 9, 32'h15, 1'b1, 32'h0);
    @(negedge clock);
    test(1'b1, 8'd14, 4'b0100 << 9, 32'h0, 1'b1, 32'h15);
    @(negedge clock);
    test(1'b1, 8'd14, 4'b0101 << 9, 32'h0, 1'b1, 32'h0);
    @(negedge clock);

    // Test block size
    $display("Block size");
    test(1'b1, 8'd14, 4'b0111 << 9, 32'h34, 1'b1, 32'h0);
    @(negedge clock);
    test(1'b1, 8'd14, 4'b0110 << 9, 32'h0, 1'b1, 32'h34);
    @(negedge clock);
    test(1'b1, 8'd14, 4'b0111 << 9, 32'h0, 1'b1, 32'h0);
    @(negedge clock);

    // Test burst size
    $display("Burst size");
    test(1'b1, 8'd14, 4'b1001 << 9, 32'h16, 1'b1, 32'h0);
    @(negedge clock);
    test(1'b1, 8'd14, 4'b1000 << 9, 32'h0, 1'b1, 32'h16);
    @(negedge clock);
    test(1'b1, 8'd14, 4'b1001 << 9, 32'h0, 1'b1, 32'h0);
    @(negedge clock);

    s_ciN = 8'b0;
    s_start = 1'b0;
    s_valueA = 32'b0;
    s_valueB = 32'b0;
    repeat (2) @(negedge clock);

    // ********  Simulate a read from the bus ********

    s_ciN = 8'd14;
    s_start = 1'b1;

    // Bus address = 0x17
    s_valueA = 4'b0011 << 9;
    s_valueB = 32'h17;
    @(negedge clock);
    // Memory address = 0x40
    s_valueA = 4'b0101 << 9;
    s_valueB = 32'h40;
    @(negedge clock);
    // Block size = 32 = 0x20
    s_valueA = 4'b0111 << 9;
    s_valueB = 32'h20;
    @(negedge clock);
    // Burst size = 8
    s_valueA = 4'b1001 << 9;
    s_valueB = 32'h8;
    @(negedge clock);

    // Start DMA
    s_valueA = 4'b1011 << 9;
    s_valueB = 32'h1;
    @(negedge clock);

    s_ciN = 8'd0;
    s_start = 1'b0;
    s_valueA = 32'b0;
    s_valueB = 32'b0;
    @(negedge clock);
    #120;

    $finish;
  end

endmodule
