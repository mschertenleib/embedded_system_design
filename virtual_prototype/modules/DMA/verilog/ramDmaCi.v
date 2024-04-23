module ramDmaCi #(
    parameter [7:0] customId = 8'h00
) (
    input  wire        start,
    clock,
    reset,
    input  wire [31:0] valueA,
    valueB,
    input  wire [ 7:0] ciN,
    output wire        done,
    output wire [31:0] result
);
  wire active = (ciN == customId) & start;
  wire address_A_valid = (valueA[31:10] == 22'd0);
  wire write_enable = active & valueA[9] & address_A_valid;
  wire read_enable = active & !valueA[9] & address_A_valid;
  wire data_ready = reading & (ciN == customId) & !start & !valueA[9] & address_A_valid;

  wire [31:0] data_out_A;
  wire [8:0] address_A = (active & address_A_valid) ? valueA[8:0] : 9'b0;

  reg reading = 1'b0;

  dualPortSSRAM #(
      .bitwidth(32),
      .nrOfEntries(512),
      .readAfterWrite(0)
  ) ssram (
      .clockA(clock),
      .clockB(1'b0),
      .writeEnableA(write_enable),
      .writeEnableB(1'b0),
      .addressA(address_A),
      .addressB(9'b0),
      .dataInA(valueB),
      .dataInB(32'b0),
      .dataOutA(data_out_A),
      .dataOutB()
  );

  assign done   = write_enable | data_ready | (active & !address_A_valid);
  assign result = data_ready ? data_out_A : 32'b0;

  always @(posedge clock) begin
    if (read_enable) reading <= 1'b1;
    if (done) reading <= 1'b0;
  end

endmodule

