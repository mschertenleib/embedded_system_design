`timescale 1ps/1ps

module ramdma_TB;

    reg cpu_start_sig,clock,reset;
    reg [7:0] cpu_Cin_sig;
    reg [31:0] valueA;
    reg [31:0] valueB;
    wire done;
    wire [31:0] result;

    initial 
    begin
        clock = 1'b0;                 /* set the initial values */
        forever #5 clock = ~clock;    /* generate a clock with a period of 10 time-units */
    end


    ramDmaCi #(
        .customId(8'd14)
    ) DUT (
        .start (cpu_start_sig),
        .valueA(valueA),
        .valueB(valueB),
        .ciN (cpu_Cin_sig),
        .done  (done),
        .result(result),
        .clock(clock),
        .reset(reset)
    );

    initial
    begin
        $dumpfile("ramDMA_Signals.vcd"); /* define the name of the .vcd file that can be viewed by GTKWAVE */
        $dumpvars(1,DUT);             /* dump all signals inside the DUT-component in the .vcd file */
    end

    initial
    begin
        valueB = 32'd0;
        valueA = 32'd0;
        cpu_start_sig = 1'b1;
        cpu_Cin_sig = 8'd14;
        repeat(10) @(negedge clock);
        repeat(255) @(negedge clock) valueA[8:0] = valueA[8:0] + 1'b1;
        valueA = 32'd0;
        repeat(255) @(negedge clock) 
        begin
            valueA[9] = 1'b1;
            valueA[8:0] = valueA[8:0] + 1'b1;
            @(negedge clock) valueA[9] = 1'b0;
        end 
        $finish;
    end

endmodule
