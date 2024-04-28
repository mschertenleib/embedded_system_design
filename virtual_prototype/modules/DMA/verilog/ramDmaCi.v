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
  wire active = (ciN == customId) ? start : 1'b0;
  wire valueA_valid = (valueA[31:13] == 19'b0);
  wire write_enable = active & (valueA[12:10] == 3'b0) & valueA[9] & valueA_valid;
  wire read_enable = active & (valueA[12:10] == 3'b0) & ~valueA[9] & valueA_valid;

  reg [31:0] bus_start_address = 32'b0;
  reg [8:0] memory_start_address = 9'b0;
  reg [9:0] block_size = 10'b0;
  reg [7:0] burst_size = 8'b0;
  reg [1:0] status_register = 2'b0;
  reg [1:0] control_register = 2'b0;

  wire [31:0] data_out_A;
  wire [8:0] address_A = (active & valueA_valid) ? valueA[8:0] : 9'b0;

  reg read_data_ready = 1'b0;

  dualPortSSRAM #(
      .bitwidth(32),
      .nrOfEntries(512),
      .readAfterWrite(0)
  ) ssram (
      .clockA(clock),
      .clockB(~clock),
      .writeEnableA(write_enable),
      .writeEnableB(1'b0),
      .addressA(address_A),
      .addressB(9'b0),
      .dataInA(valueB),
      .dataInB(32'b0),
      .dataOutA(data_out_A),
      .dataOutB()
  );

  always @(posedge clock) begin
    if (read_enable) read_data_ready <= 1'b1;
    if (done) read_data_ready <= 1'b0;
    if (active & valueA_valid & valueA[9]) begin
      if (valueA[12:10] == 3'b001) bus_start_address <= valueB[31:0];
      else if (valueA[12:10] == 3'b010) memory_start_address <= valueB[8:0];
      else if (valueA[12:10] == 3'b011) block_size <= valueB[9:0];
      else if (valueA[12:10] == 3'b100) burst_size <= valueB[7:0];
      else if (valueA[12:10] == 3'b101) control_register <= valueB[1:0];
    end
  end

  assign done = read_data_ready | (active & ~read_enable);

  assign result = (active & valueA_valid & ~valueA[9]) ?
      ((valueA[12:10] == 3'b001) ? bus_start_address :
      (valueA[12:10] == 3'b010) ? memory_start_address :
      (valueA[12:10] == 3'b011) ? block_size :
      (valueA[12:10] == 3'b100) ? burst_size :
      (valueA[12:10] == 3'b101) ? status_register : 32'b0)
    : read_data_ready ? data_out_A : 32'b0;


endmodule

