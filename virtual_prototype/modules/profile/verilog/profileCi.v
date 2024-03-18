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
  wire cycleCounter2Enable;
  reg  cycleCounterEnableLatch;
  reg  stallCounterEnableLatch;
  reg  busIdleCounterEnableLatch;
  reg  cycleCounter2EnableLatch;
  assign cycleCounterEnable = (reset == 1'b1) ? 1'b0 :
                          (valueB[4] == 1'b1) ? 1'b0 :
                          (valueB[0] == 1'b1) ? 1'b1 :
                          cycleCounterEnableLatch;
  assign stallCounterEnable = (reset == 1'b1) ? 1'b0 :
                          (valueB[5] == 1'b1) ? 1'b0 :
                          (valueB[1] == 1'b1) ? 1'b1 :
                          stallCounterEnableLatch;
  assign busIdleCounterEnable = (reset == 1'b1) ? 1'b0 :
                            (valueB[6] == 1'b1) ? 1'b0 :
                            (valueB[2] == 1'b1) ? 1'b1 :
                            busIdleCounterEnableLatch;
  assign cycleCounter2Enable = (reset == 1'b1) ? 1'b0 :
                          (valueB[7] == 1'b1) ? 1'b0 :
                          (valueB[3] == 1'b1) ? 1'b1 :
                          cycleCounter2EnableLatch;

  always @(posedge clock) begin
    cycleCounterEnableLatch   <= cycleCounterEnable;
    stallCounterEnableLatch   <= stallCounterEnable;
    busIdleCounterEnableLatch <= busIdleCounterEnable;
    cycleCounter2EnableLatch  <= cycleCounter2Enable;
  end

  wire cycleCounterReset;
  wire stallCounterReset;
  wire busIdleCounterReset;
  wire cycleCounter2Reset;
  assign cycleCounterReset   = reset | valueB[8];
  assign stallCounterReset   = reset | valueB[9];
  assign busIdleCounterReset = reset | valueB[10];
  assign cycleCounter2Reset  = reset | valueB[11];

  wire [31:0] cycleCounterValue;
  wire [31:0] stallCounterValue;
  wire [31:0] busIdleCounterValue;
  wire [31:0] cycleCounter2Value;

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

  counter #(
      .WIDTH(32)
  ) cycleCounter2 (
      .reset(cycleCounter2Reset),
      .clock(clock),
      .enable(cycleCounter2Enable),
      .direction(1'b1),
      .counterValue(cycleCounter2Value)
  );

  assign result = (ciN == customId && start == 1'b1) ?
    ((valueA[1:0] == 2'd0) ? cycleCounterValue :
     (valueA[1:0] == 2'd1) ? stallCounterValue :
     (valueA[1:0] == 2'd2) ? busIdleCounterValue :
                             cycleCounterValue)
    : 32'd0;

  assign done = (ciN == customId && start == 1'b1) ? 1'b1 : 1'b0;

endmodule
