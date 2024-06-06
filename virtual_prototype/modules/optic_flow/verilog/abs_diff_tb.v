`timescale 1ps / 1ps

module absDiffCI_tb;

    reg start;
    reg [7:0] ciN;
    reg [31:0] valueA;
    reg [31:0] valueB;
    wire done;
    wire [31:0] result;

    absDiffCI #(
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
                "\033[1;31mstart=%b, ciN=%0d, valueA=0x%0h, valueB=0x%0h => done=%b (expected %b), result=0x%0h (expected 0x%0h)\033[m",
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
    
        // Test computation
        test_ci(.test_start(1'b1), .test_ciN(8'd30), .test_valueA({8'd40, 8'd17, 8'd17, 8'd19}), .test_valueB(32'd0),
                .expected_done(1'b1), .expected_result({30'b0, 1'b1, 1'b0}));
        test_ci(.test_start(1'b1), .test_ciN(8'd30), .test_valueA({8'd31, 8'd37, 8'd17, 8'd19}), .test_valueB(32'd0),
                .expected_done(1'b1), .expected_result({30'b0, 1'b0, 1'b0}));
        test_ci(.test_start(1'b1), .test_ciN(8'd30), .test_valueA({8'd04, 8'd57, 8'd37, 8'd19}), .test_valueB(32'd0),
                .expected_done(1'b1), .expected_result({30'b0, 1'b1, 1'b1}));

        $finish;
    end

endmodule
