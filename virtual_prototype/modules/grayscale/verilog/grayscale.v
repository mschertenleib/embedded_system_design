
module rgb565GrayscaleIse #(parameter [7:0] customInstructionId = 8'd0)
    (input wire        start,
    input wire[31:0]   valueB,valueA, //rgb565value (lower 16bits)
    input wire[7:0]    iseId,
    output wire        done,
    output wire[31:0]  result);

    wire s_isMyCi = (iseId == customInstructionId) ? start : 1'b0;
    assign done = s_isMyCi; // delay needed ?

    wire[4:0] red_ch, blue_ch;
    wire[5:0] green_ch;
    assign red_ch = valueA[15:11];
    assign green_ch = valueA[10:5];
    assign blue_ch = valueA[4:0];

    wire[7:0] red_g, grn_g, blu_g;
    assign red_g = (red_ch >> 3) + (red_ch >> 4) + (red_ch >> 6) + (red_ch >> 7); // 54/256 = (32 + 16 + 4 +2)/256 = x>>3 + x>>4 + x>>6 + x>>7 
    assign grn_g = (green_ch >> 1) + (green_ch >> 3) + (green_ch >> 4) + (green_ch >> 5);
    assign blu_g = (blue_ch >> 4) + (blue_ch >> 7) + (blue_ch >> 8); //

    assign result[31:8] = 24'd0;
    assign result[7:0] = red_g + grn_g + blu_g;
    

endmodule