
module ramDmaCi #(parameter[7:0] customId = 8'h00)
               (input wire          start,clock,reset,
                input wire[31:0]    valueA,valueB,
                input wire[7:0]     ciN,
                output wire         done,
                output wire[31:0]   result);
// internal registers
reg[31:0] memory [511:0];
reg[8:0] ptrA;
// state registers
reg busy = 0;

wire s_isMyCi = (ciN == customId) ? start : 1'b0;
wire writea_en = valueA[9];                 // 1 bit write enabled
wire validA = (valueA[31:10] == 22'd0);     // last 22bits must be 0 to write

assign done = s_isMyCi & !busy; // not always true ?

always @(posedge clock)
begin
    if(busy)
    begin
        memory[ptrA] = valueA;
        busy <= 1'b0;
    end
    else ptrA <= valueA[8:0];

    if(writea_en & validA) busy = 1'b1;

    if(reset == 1'b1)begin
        for(integer i = 0 ; i < 512 ; i=i+1 ) begin
            memory[i] = 32'd0;
        end
    end
end

assign result = (s_isMyCi) ? memory[ptrA] : 32'd0;

endmodule