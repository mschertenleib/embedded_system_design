module dualPortSSRAM #(
    parameter bitwidth = 8,
    parameter nrOfEntries = 512
) (
    input wire clock,
    writeEnableA,
    writeEnableB,
    input wire [$clog2(nrOfEntries)-1 : 0] addressA,
    addressB,
    input wire [bitwidth-1 : 0] dataInA,
    dataInB,
    output wire [bitwidth-1 : 0] dataOutA,
    dataOutB
);

  reg [bitwidth-1 : 0] memoryContent[nrOfEntries];

  assign dataOutA = memoryContent[addressA];
  assign dataOutB = memoryContent[addressB];

  always @(posedge clock) begin
    if (writeEnableA == 1'b1) memoryContent[addressA] = dataInA;
  end
  always @(negedge clock) begin
    if (writeEnableB == 1'b1) memoryContent[addressB] = dataInB;
  end
endmodule
