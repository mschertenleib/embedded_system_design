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

  wire cycleCounterEnable;
  wire stallCounterEnable;
  wire busIdleCounterEnable;

  // FIXME: I can't see how this would work
  assign cycleCounterEnable = (valueB[4] == 1'b1) ? 1'b0 :
                          (valueB[0] == 1'b1) ? 1'b1 :
                          cycleCounterEnable;
  assign stallCounterEnable = (valueB[5] == 1'b1) ? 1'b0 :
                          (valueB[1] == 1'b1) ? 1'b1 :
                          stallCounterEnable;
  assign busIdleCounterEnable = (valueB[6] == 1'b1) ? 1'b0 :
                            (valueB[2] == 1'b1) ? 1'b1 :
                            busIdleCounterEnable;

  wire cycleCounterReset;
  wire stallCounterReset;
  wire busIdleCounterReset;
  assign cycleCounterReset   = valueB[8];
  assign stallCounterReset   = valueB[9];
  assign busIdleCounterReset = valueB[10];

  reg [31:0] cycleCounterValue;
  reg [31:0] stallCounterValue;
  reg [31:0] busIdleCounterValue;

  counter #(
      .WIDTH(32)
  ) cycleCounter (
      .reset(cycleCounterReset),
      .clock(clock),
      .enable(cycleCounterEnable),
      .direction(1'b1),
      .counterValue(cycleCounterValue)
  );

  counter #(
      .WIDTH(32)
  ) stallCounter (
      .reset(stallCounterReset),
      .clock(clock),
      .enable(stallCounterEnable & stall),
      .direction(1'b1),
      .counterValue(stallCounterValue)
  );

  counter #(
      .WIDTH(32)
  ) busIdleCounter (
      .reset(busIdleCounterReset),
      .clock(clock),
      .enable(busIdleCounterEnable & busIdle),
      .direction(1'b1),
      .counterValue(busIdleCounterValue)
  );

  assign result = (ciN == customId && start == 1'b1) ?
    ((valueA[1:0] == 2'd0) ? cycleCounterValue :
     (valueA[1:0] == 2'd1) ? stallCounterValue :
     (valueA[1:0] == 2'd2) ? busIdleCounterValue :
                             cycleCounterValue)
    : 32'd0;

  assign done = (ciN == customId && start == 1'b1) ? 1'b1 : 1'b0;

endmodule

