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
    output reg [bitwidth-1 : 0] dataOutA,
    output reg [bitwidth-1 : 0] dataOutB
);

  reg [bitwidth-1 : 0] memoryContent[nrOfEntries];

  always @(posedge clock) begin
    if (writeEnableA == 1'b1) memoryContent[addressA] <= dataInA;
    if (writeEnableB == 1'b1) memoryContent[addressB] <= dataInB;
  end
  always @(negedge clock) begin
    dataOutA <= memoryContent[addressA];
    dataOutB <= memoryContent[addressB];
  end
endmodule

