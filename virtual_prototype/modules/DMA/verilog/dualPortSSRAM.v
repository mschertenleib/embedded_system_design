module dualPortSSRAM #(
    parameter bitwidth = 8,
    parameter nrOfEntries = 512,
    parameter readAfterWrite = 0
) (
    input wire clockA,
    clockB,
    writeEnableA,
    writeEnableB,
    input wire [$clog2(nrOfEntries)-1 : 0] addressA,
    addressB,
    input wire [bitwidth-1 : 0] dataInA,
    dataInB,
    output reg [bitwidth-1 : 0] dataOutA,
    dataOutB
);

  reg [bitwidth-1 : 0] memoryContent[nrOfEntries];

  always @(posedge clockA) begin
    if (readAfterWrite == 0) begin
      dataOutA <= memoryContent[addressA];
      if (writeEnableA == 1'b1) memoryContent[addressA] = dataInA;
    end else begin
      if (writeEnableA == 1'b1) memoryContent[addressA] = dataInA;
      dataOutA <= memoryContent[addressA];
    end
  end
  always @(posedge clockB) begin
    if (readAfterWrite == 0) begin
      dataOutB <= memoryContent[addressB];
      if (writeEnableB == 1'b1) memoryContent[addressB] = dataInB;
    end else begin
      if (writeEnableB == 1'b1) memoryContent[addressB] = dataInB;
      dataOutB <= memoryContent[addressB];
    end
  end
endmodule
