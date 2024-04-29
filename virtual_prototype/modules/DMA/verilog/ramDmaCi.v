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
  wire write_enable_A = active & (valueA[12:10] == 3'b0) & valueA[9] & valueA_valid;
  wire read_enable_A = active & (valueA[12:10] == 3'b0) & ~valueA[9] & valueA_valid;

  reg [31:0] bus_start_address = 32'b0;
  reg [8:0] memory_start_address = 9'b0;
  reg [9:0] block_size = 10'b0;
  reg [7:0] burst_size = 8'b0;
  reg [1:0] status_register = 2'b0;
  reg [1:0] control_register = 2'b0;

  wire [31:0] data_out_A;
  wire [31:0] data_out_B;
  wire [8:0] address_A = (active & valueA_valid) ? valueA[8:0] : 9'b0;
  wire write_enable_B = ((state == READING) & data_valid_in) ? 1'b1 : 1'b0;
  wire [31:0] data_in_B = ((state == READING) & data_valid_in) ? address_data_in : 32'b0;

  reg data_valid_out_B = 1'b0;

  reg [31:0] data_out_B_reg;

  dualPortSSRAM #(
      .bitwidth(32),
      .nrOfEntries(512)
  ) ssram (
      .clock(clock),
      .writeEnableA(write_enable_A),
      .writeEnableB(write_enable_B),
      .addressA(address_A),
      .addressB(memory_start_address),
      .dataInA(valueB),
      .dataInB(data_in_B),
      .dataOutA(data_out_A),
      .dataOutB(data_out_B)
  );


  // ******* FSM *******

  localparam [2:0] IDLE = 3'd0,
    REQUEST_READ = 3'd1,
    REQUEST_WRITE = 3'd2,
    START_READ = 3'd3,
    START_WRITE = 3'd4,
    READING = 3'd5,
    WRITING = 3'd6,
    ERROR = 3'd7;

  reg [2:0] state = IDLE;
  reg [9:0] block_count = 10'b0;
  reg [7:0] burst_count = 8'b0;

  always @(posedge clock) begin
    case (state)
      IDLE:
      state <= (block_size != 0) ?
        ((control_register[1:0] == 2'b01) ?
          REQUEST_READ
          : (control_register[1:0] == 2'b10) ?
            REQUEST_WRITE
            : state)
        : state;
      REQUEST_READ: state <= granted ? START_READ : state;
      REQUEST_WRITE: state <= granted ? START_WRITE : state;
      START_READ: state <= READING;
      START_WRITE: state <= WRITING;
      READING: begin
        if (error_in) state <= ERROR;
        else if ((burst_count == burst_size + 8'd1) & end_transaction_in) begin  // End of burst
          if (block_count == block_size) state <= IDLE;  // End of transfer
          else state <= REQUEST_READ;  // Next burst
        end
      end
      WRITING: begin
        if (error_in) state <= ERROR;
        else if (burst_count == burst_size + 8'd1) begin  // End of burst
          if (block_count == block_size) state <= IDLE;  // End of transfer
          else state <= REQUEST_WRITE;  // Next burst
        end
      end
      ERROR: state <= IDLE;
      default: state <= IDLE;
    endcase
  end


  // ******* Other flip-flop updates *******

  always @(posedge clock) begin
    // Config registers update
    if (active & valueA_valid & valueA[9] & (state == IDLE)) begin
      case (valueA[12:10])
        3'b001:  bus_start_address <= valueB[31:0];
        3'b010:  memory_start_address <= valueB[8:0];
        3'b011:  block_size <= valueB[9:0];
        3'b100:  burst_size <= valueB[7:0];
        3'b101:  control_register <= valueB[1:0];
        default: ;
      endcase
    end

    if (state == REQUEST_WRITE) begin
      data_valid_out_B <= 1'b0;
    end else if (state == READING) begin
      if (data_valid_in) begin
        block_count <= block_count + 10'd1;
        burst_count <= burst_count + 8'd1;
        bus_start_address <= bus_start_address + 32'd4;
        memory_start_address <= memory_start_address + 9'd4;
      end
    end else if (state == WRITING) begin
      data_valid_out_B <= 1'b1;
      data_out_B_reg   <= data_out_B;
      if (~busy_in & (burst_count != burst_size + 8'd1)) begin
        block_count <= block_count + 10'd1;
        burst_count <= burst_count + 8'd1;
        bus_start_address <= bus_start_address + 32'd4;
        memory_start_address <= memory_start_address + 9'd4;
      end
    end else if (state == IDLE) begin
      data_valid_out_B <= 1'b0;
      burst_count <= 8'd0;
      block_count <= 10'd0;
    end else begin
      burst_count <= 8'd0;
    end

    status_register[0] <= (state == IDLE) ? 1'b0 : 1'b1;
    status_register[1] <= (state == ERROR) ?
      1'b1
      : ((state == REQUEST_READ) | (state == REQUEST_WRITE)) ?
        1'b0
        : status_register[1];
    if (state != IDLE) control_register <= 2'b00;
  end

  assign done = active;

  assign result = (active & valueA_valid & ~valueA[9]) ?
      ((valueA[12:10] == 3'b000) ? data_out_A :
      (valueA[12:10] == 3'b001) ? bus_start_address :
      (valueA[12:10] == 3'b010) ? memory_start_address :
      (valueA[12:10] == 3'b011) ? block_size :
      (valueA[12:10] == 3'b100) ? burst_size :
      (valueA[12:10] == 3'b101) ? status_register : 32'b0)
      : 32'b0;


  assign request = ((state == REQUEST_READ) | (state == REQUEST_WRITE)) ? 1'b1 : 1'b0;
  assign address_data_out = ((state == START_READ) | (state == START_WRITE))
    ? bus_start_address
    : ((state == WRITING) & data_valid_out_B)
      ? data_out_B_reg
      : 32'b0;
  assign byte_enables_out = ((state == START_READ) | (state == START_WRITE)) ? 4'b1111 : 4'b0;
  assign burst_size_out = ((state == START_READ) | (state == START_WRITE)) ? burst_size : 8'b0;
  assign read_n_write_out = (state == START_READ) ? 1'b1 : 1'b0;
  assign begin_transaction_out = ((state == START_READ) | (state == START_WRITE)) ? 1'b1 : 1'b0;
  assign end_transaction_out = ((state == ERROR)
    | ((state == WRITING) & (burst_count == burst_size + 8'd1)))
    ? 1'b1
    : 1'b0;
  assign data_valid_out = data_valid_out_B;


endmodule

