module opticFlowCI #(
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

    wire[31:0] optic_flow_values;

    assign done   = is_active;
    assign result = is_active ? optic_flow_values : 32'd0;

    /*
        ValueA contains two rows of 8*2 bits each, containing the binary X and Y gradients for 8 pixels
        ValueB is identical, but the gradients come from the previous frame
        Result contains 8*4 bits, containing the {up, down, left, right} flow bits for each pixel
    */

    wire[15:0] row_up = valueA[31:16];
    wire[15:0] row_down = valueA[15:0];
    wire[15:0] prev_row_up = valueB[31:16];
    wire[15:0] prev_row_down = valueB[15:0];

    wire[7:0] row_up_x;
    wire[7:0] prev_row_up_x;
    wire[7:0] row_up_y;
    wire[7:0] row_down_y;
    wire[7:0] prev_row_up_y;
    wire[7:0] prev_row_down_y;
    genvar i;
    generate
        for (i = 0; i < 8; i = i + 1) begin
            assign row_up_x[i] = row_up[2 * i];
            assign prev_row_up_x[i] = prev_row_up[2 * i];
            assign row_up_y[i] = row_up[2 * i + 1];
            assign row_down_y[i] = row_down[2 * i + 1];
            assign prev_row_up_y[i] = prev_row_up[2 * i + 1];
            assign prev_row_down_y[i] = prev_row_down[2 * i + 1];
        end
    endgenerate

    wire[7:0] left_and = row_up_x & (prev_row_up_x >> 1);
    wire[7:0] right_and = (row_up_x >> 1) & prev_row_up_x;
    wire[7:0] left = left_and & ~right_and;
    wire[7:0] right = right_and & ~left_and;

    wire[7:0] up_and = row_up_y & prev_row_down_y;
    wire[7:0] down_and = row_down_y & prev_row_up_y;
    wire[7:0] up = up_and & ~down_and;
    wire[7:0] down = down_and & ~up_and;

    generate
        for (i = 0; i < 8; i = i + 1) begin
            assign optic_flow_values[4 * i] = right[i];
            assign optic_flow_values[4 * i + 1] = left[i];
            assign optic_flow_values[4 * i + 2] = down[i];
            assign optic_flow_values[4 * i + 3] = up[i];
        end
    endgenerate

endmodule
