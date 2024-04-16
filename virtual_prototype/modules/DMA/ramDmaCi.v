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
  reg  [31:0] memory                                                 [512];
  reg  [ 8:0] address;
  reg         in_a_write;

  wire        active = (ciN == customId) ? start : 1'b0;
  wire        write_enable = valueA[9];
  wire        address_A_valid = (valueA[31:10] == 22'd0);
  wire        read_enable = (active & !in_a_write & address_A_valid);

  always @(posedge clock) begin
    if (active & address_A_valid & !in_a_write & write_enable) begin
      // Set the address
      address <= valueA[8:0];
      in_a_write <= 1'b1;
    end else if (active & in_a_write & write_enable) begin
      // Store the data
      memory[address] <= valueA;
      in_a_write <= 1'b0;
    end
  end

  assign result = read_enable ? memory[valueA[8:0]] : 32'd0;

endmodule

