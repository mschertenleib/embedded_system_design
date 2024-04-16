
module ramDmaCi #(parameter[7:0] customId = 8'h00)
               (input wire          start,clock,reset,
                input wire[31:0]    valueA,valueB,
                input wire[7:0]     ciN,
                output wire         done,
                output wire[31:0]   result);
// internal components
wire[31:0]  dataA,dataB,outA,outB;
wire[8:0]   addrA,addrB;
wire        writeA,writeB;

// internal dualport sram
dualPortSSRAM #() internalmem ( .clockA(clock),
                            .clockB(clock),
                            .writeEnableA(writeA),
                            .writeEnableB(writeB),
                            .addressA(addrA),
                            .addressB(addrB),
                            .dataInA(dataA),
                            .dataInB(dataB),
                            .dataOutA(outA),
                            .dataOutB(outB));


// bus interface
assign writeB = 1'b0;
assign dataB = 32'd0;
assign addrB = 8'd0;

//CPU interface
reg isreading;
reg[31:0] sramout;
wire isvalid = (valueA[31:10] == 22'd0) ? 1'b1 : 1'b0;

wire s_isMyCi = (ciN == customId) ? start : 1'b0;
assign done = s_isMyCi && !isreading;

assign writeA = valueA[9] && isvalid;
assign addrA = valueA[8:0];
assign dataA = valueB;


@(posedge clock)begin
    if(reset || writeA)begin
         isreading = 1'b0;
         sramout = 32'b0;
    end
    if(!writeA && !isreading) isreading = 1'b1;
    else isreading = 1'b0;

    if(isreading) sramout = outA;
end




endmodule