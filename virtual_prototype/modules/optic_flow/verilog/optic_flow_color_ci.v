module opticFlowColorCI #(
    parameter [7:0] customInstructionId = 8'd0
) (
    input  wire        start,
    input  wire [31:0] valueA,
    valueB,
    input  wire [ 7:0] ciN,
    output wire        done,
    output wire [31:0] result
);
    wire is_active = (ciN == customInstructionId) ? start : 1'b0;

    wire[31:0] rgb565;

    assign done   = is_active;
    assign result = is_active ? rgb565 : 32'd0;

    /*
        ValueA contains 2*4 bits (LSBs) corresponding to the {up, down, left, right} flow bits for two pixels
        Result contains RGB565 colors for these two pixels
    */

    wire right_0 = valueA[0];
    wire left_0 = valueA[1];
    wire down_0 = valueA[2];
    wire up_0 = valueA[3];
    wire right_1 = valueA[4];
    wire left_1 = valueA[5];
    wire down_1 = valueA[6];
    wire up_1 = valueA[7];
    wire[4:0] red_0 = (left_0 << 4) | (down_0 << 4);
    wire[5:0] green_0 = (right_0 << 5) | (down_0 << 5);
    wire[4:0] blue_0 = (up_0 << 4) | (down_0 << 4);
    wire[4:0] red_1 = (left_1 << 4) | (down_1 << 4);
    wire[5:0] green_1 = (right_1 << 5) | (down_1 << 5);
    wire[4:0] blue_1 = (up_1 << 4) | (down_1 << 4);
    assign rgb565 = {red_1, green_1, blue_1, red_0, green_0, blue_0};

endmodule
