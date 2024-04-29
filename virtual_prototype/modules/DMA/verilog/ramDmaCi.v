module ramDmaCi #(
    parameter [7:0] customId = 8'h00
) (
    // CI interface
    input  wire        start,
    clock,
    reset,
    input  wire [31:0] valueA,
    valueB,
    input  wire [ 7:0] ciN,
    output wire        done,
    output wire [31:0] result,

    // Bus interface
    input wire granted,
    input wire [31:0] address_data_in,
    input wire end_transaction_in,
    data_valid_in,
    busy_in,
    error_in,
    output wire request,
    output wire [31:0] address_data_out,
    output wire [3:0] byte_enables_out,
    output wire [7:0] burst_size_out,
    output wire read_n_write_out,
    begin_transaction_out,
    end_transaction_out,
    data_valid_out
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
      case (valueA[12:10])
        3'b001:  bus_start_address <= valueB[31:0];
        3'b010:  memory_start_address <= valueB[8:0];
        3'b011:  block_size <= valueB[9:0];
        3'b100:  burst_size <= valueB[7:0];
        3'b101:  if (state == IDLE) control_register <= valueB[1:0];
        default: ;
      endcase
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



  localparam [2:0] IDLE = 3'd0, REQUEST = 3'd1, START_READ = 3'd2, READING = 3'd3, ERROR = 3'd5;

  reg [2:0] state = IDLE;
  reg [9:0] block_count = 10'b0;
  reg [7:0] burst_count = 8'b0;

  always @(posedge clock) begin
    case (state)
      IDLE: state <= (control_register[0] & (block_size != 0)) ? REQUEST : state;
      REQUEST: state <= granted ? START_READ : state;
      START_READ: state <= READING;
      READING: begin
        if (error_in) state <= ERROR;
        else if ((burst_count == burst_size + 8'd1) & end_transaction_in) begin  // End of burst
          if (block_count == block_size) state <= IDLE;  // End of transfer
          else state <= REQUEST;  // Next burst
        end
      end
      ERROR: state <= IDLE;
      default: state <= IDLE;
    endcase
  end



  assign request = (state == REQUEST) ? 1'b1 : 1'b0;
  assign address_data_out = (state == START_READ) ? bus_start_address : 32'b0;
  assign byte_enables_out = (state == START_READ) ? 4'b1111 : 4'b0;
  assign burst_size_out = (state == START_READ) ? burst_size : 8'b0;
  assign read_n_write_out = (state == START_READ) ? 1'b1 : 1'b0;
  assign begin_transaction_out = (state == START_READ) ? 1'b1 : 1'b0;
  assign end_transaction_out = (state == ERROR) ? 1'b1 : 1'b0;
  assign data_valid_out = 1'b0;


  always @(posedge clock) begin
    if (state == READING) begin
      if (data_valid_in) begin
        block_count <= block_count + 10'd1;
        burst_count <= burst_count + 8'd1;
        bus_start_address <= bus_start_address + 32'd4;
        memory_start_address <= memory_start_address + 9'd4;
      end
    end else if (state == IDLE) begin
      burst_count <= 8'd0;
      block_count <= 10'd0;
    end else burst_count <= 8'd0;
    status_register[0] <= (state == IDLE) ? 1'b0 : 1'b1;
    status_register[1] <= (state == ERROR) ? 1'b1 : (state == REQUEST) ? 1'b0 : status_register[1];
    if (state != IDLE) control_register <= 1'b0;
  end

endmodule

