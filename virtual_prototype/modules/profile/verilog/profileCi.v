module profileCi #(
    parameter [7:0] customId = 8'h00
) (
    input wire start,
    clock,
    reset,
    stall,
    busIdle,
    input wire [31:0] valueA,
    valueB,
    input wire [7:0] ciN,
    output wire done,
    output wire [31:0] result
);

  wire [3:0] counterEnables;
  wire [3:0] counterResets;

  assign counterEnables[0] = (valueB[0] == 1'b1 && valueB[4] == 1'b0) ? 1'b1 : 1'b0;
  assign counterEnables[1] = (valueB[1] == 1'b1 && valueB[5] == 1'b0 && stall == 1'b1) ? 1'b1 : 1'b0;
  assign counterEnables[2] = (valueB[2] == 1'b1 && valueB[6] == 1'b0 && busIdle == 1'b1) ? 1'b1 : 1'b0;
  assign counterEnables[3] = (valueB[3] == 1'b1 && valueB[7] == 1'b0) ? 1'b1 : 1'b0;
  assign counterResets[0] = valueB[8];
  assign counterResets[1] = valueB[9];
  assign counterResets[2] = valueB[10];
  assign counterResets[3] = valueB[11];

  reg [127:0] counterValues;

  counter #(
      .WIDTH(32)
  ) counters[3:0] (
      .reset(counterResets),
      .clock(clock),
      .enable(counterEnables),
      .direction(1'b1),
      .counterValue(counterValues)
  );

  assign result = (ciN == customId && start == 1'b1) ?
    (valueA[1:0] == 2'd0 ? counterValues[31:0] :
    (valueA[1:0] == 2'd1 ? counterValues[63:32] :
    (valueA[1:0] == 2'd2 ? counterValues[95:64] :
                           counterValues[127:96])))
                           : 32'd0;

  assign done = (ciN == customId && start == 1'b1) ? 1'b1 : 1'b0;

endmodule

