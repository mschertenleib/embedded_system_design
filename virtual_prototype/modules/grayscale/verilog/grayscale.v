`define rgb2gray(redch,greench,bluech) \
    (``redch`` >> 3) + (``redch`` >> 4) + (``redch`` >> 6) + (``redch`` >> 7) +\
    (``greench`` >> 1) + (``greench`` >> 3) + (``greench`` >> 4) + (``greench`` >> 5) +\
    (``bluech`` >> 4) + (``bluech`` >> 7) + (``bluech`` >> 8)


module rgb565GrayscaleIse #(parameter [7:0] customInstructionId = 8'd13)
    (input wire        start,
    input wire[31:0]   valueB,valueA, //rgb565value (lower 16bits)
    input wire[7:0]    iseId,
    output wire        done,
    output wire[31:0]  result);

    wire s_isMyCi = (iseId == customInstructionId) ? start : 1'b0;
    assign done = s_isMyCi;

    wire[4:0] red_ch, blue_ch;
    wire[5:0] green_ch;

    assign red_ch = valueA[15:11];
    assign green_ch = valueA[10:5];
    assign blue_ch = valueA[4:0];

    assign result[31:8] = 24'd0;
    assign result[7:0] = s_isMyCi ? rgb2gray(red_ch,green_ch,blue_ch) : 8'd0;
    
endmodule

module rgb2grayParallelIse #(parameter [7:0] customInstructionId = 8'd14)
    (input wire        start,
    input wire[31:0]   valueB,valueA, //valA[pixel0,pixel1] valB[pixel2,pixel3]
    input wire[7:0]    iseId,
    output wire        done,
    output wire[31:0]  result);

    wire s_isMyCi = (iseId == customInstructionId) ? start : 1'b0;
    assign done = s_isMyCi;

    wire[15:0]   px0 = valueA[31:16]
                ,px1 = valueA[15:0]
                ,px2 = valueB[31:16]
                ,px3 = valueB[15:0];
    wire[5:0]    px0r = px0[15:11]
                ,px0b = px0[4:0]
                ,px1r = px1[15:11]
                ,px1b = px1[4:0]
                ,px2r = px2[15:11]
                ,px2b = px2[4:0]
                ,px3r = px3[15:11]
                ,px3b = px3[4:0];
    wire[6:0]    px0g = px0[10:5]
                ,px1g = px1[10:5]
                ,px2g = px2[10:5]
                ,px3g = px3[10:5];

    assign result[7:0] = s_isMyCi ? `rgb2gray(px0r,px0g,px0b) : 8'd0;
    assign result[15:8] = s_isMyCi ? `rgb2gray(px1r,px1g,px1b) : 8'd0;
    assign result[23:16] = s_isMyCi ? `rgb2gray(px2r,px2g,px2b) : 8'd0;
    assign result[31:24] = s_isMyCi ? `rgb2gray(px3r,px3g,px3b) : 8'd0;

endmodule
