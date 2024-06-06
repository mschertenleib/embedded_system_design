module absDiffCI #(
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

    wire[31:0] abs_diff;

    assign done   = is_active;
    assign result = is_active ? abs_diff : 32'd0;

    wire [7:0] gray_left = valueA[7:0];
    wire [7:0] gray_right = valueA[15:8];
    wire [7:0] gray_up = valueA[23:16];
    wire [7:0] gray_down = valueA[31:24];

    wire dx = (gray_right >= gray_left)
        ? (gray_right - gray_left > 8'd10 ? 1'b1 : 1'b0)
        : (gray_left - gray_right > 8'd10 ? 1'b1 : 1'b0);
    wire dy = (gray_up >= gray_down)
        ? (gray_up - gray_down > 8'd10 ? 1'b1 : 1'b0)
        : (gray_down - gray_up > 8'd10 ? 1'b1 : 1'b0);
    assign abs_diff = {30'd0, dy, dx};

endmodule
