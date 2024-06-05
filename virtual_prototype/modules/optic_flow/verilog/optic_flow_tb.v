`timescale 1ps / 1ps

module opticFlowCI_tb;

    reg start;
    reg [7:0] ciN;
    reg [31:0] valueA;
    reg [31:0] valueB;
    wire done;
    wire [31:0] result;

    reg [15:0] row_up;
    reg [15:0] prev_row_up;
    reg [15:0] row_down;
    reg [15:0] prev_row_down;
    reg [31:0] exp_result;


    opticFlowCI #(
        .customInstructionId(8'd30)
    ) dut (
        .start (start),
        .valueA(valueA),
        .valueB(valueB),
        .ciN   (ciN),
        .done  (done),
        .result(result)
    );

    task automatic test_ci;
        input test_start;
        input [7:0] test_ciN;
        input [31:0] test_valueA, test_valueB;
        input expected_done;
        input [31:0] expected_result;
        begin
        start = test_start;
        valueA = test_valueA;
        valueB = test_valueB;
        ciN = test_ciN;
        #1
        if (done == expected_done && result == expected_result) begin
            $display(
                "\033[1;32mstart=%b, ciN=%0d, valueA=%0h, valueB=%0h => done=%b (expected %b), result=%0h (expected %0h)\033[m",
                start,
                ciN,
                valueA,
                valueB,
                done,
                expected_done,
                result,
                expected_result
            );
        end else begin
            $display(
                "\033[1;31mstart=%b, ciN=%0d, valueA=%0h, valueB=%0h => done=%b (expected %b), result=%0h (expected %0h)\033[m",
                start,
                ciN,
                valueA,
                valueB,
                done,
                expected_done,
                result,
                expected_result
            );
        end
        end
    endtask

    initial begin
        // Test start/done/result logic
        test_ci(.test_start(1'b0), .test_ciN(8'd47), .test_valueA(32'd0), .test_valueB(32'd0),
                .expected_done(1'b0), .expected_result(32'd0));
        test_ci(.test_start(1'b1), .test_ciN(8'd47), .test_valueA(32'd0), .test_valueB(32'd0),
                .expected_done(1'b0), .expected_result(32'd0));
        test_ci(.test_start(1'b0), .test_ciN(8'd30), .test_valueA(32'd0), .test_valueB(32'd0),
                .expected_done(1'b0), .expected_result(32'd0));
    
        /*
            Test computation (from reference C implementation)

        row_up =        0b0010010110101000;
        prev_row_up =   0b1001000100100100;
        row_down =      0b1100100100010000;
        prev_row_down = 0b0101010010100100;
      
            Result:

        up =    0b0000010010100000
        down =  0b1000000100000000
        left =  0b0010010000001000
        right = 0b0000000100100000

        up =     0   0   0   0   1   1   0   0
        down =    1   0   0   0   0   0   0   0
        left =     0   0   1   0   0   0   0   0
        right =     0   0   0   1   0   0   0   0
        result = 01000000001000011000100000000000
        */

        assign row_up =        16'b0010010110101000;
        assign prev_row_up =   16'b1001000100100100;
        assign row_down =      16'b1100100100010000;
        assign prev_row_down = 16'b0101010010100100;
        valueA <= {row_up, row_down};
        valueB <= {prev_row_up, prev_row_down};
        assign exp_result = 32'b01000000001000011000100000000000;

        test_ci(.test_start(1'b1), .test_ciN(8'd30), .test_valueA(valueA), .test_valueB(valueB),
                .expected_done(1'b1), .expected_result(exp_result));

        $finish;
    end

endmodule
