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
        reset = 1'b1;
        clock = 1'b0;                 /* set the initial values */
        repeat (4) #5 clock = ~clock; /* generate 2 clock periods */
        reset = 1'b0;                 /* de-activate the reset */
        forever #5 clock = ~clock;    /* generate a clock with a period of 10 time-units */
    end

    ramDmaCi #(
        .customId(8'h0F)
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
        cpu_Cin_sig = 8'h0F;
        @(negedge reset);

        valueA[9] = 1'b1;
        repeat(10) @(negedge clock) begin
            valueA[8:0] = valueA[8:0] + 1'b1;
            valueB = valueB + 1'b1;
        end

        valueB = 32'd0;
        valueA = 32'd0;
        repeat(10) @(negedge clock) begin
            @(negedge clock);
            valueA[8:0] = valueA[8:0] + 1'b1;
        end

        $finish;
    end

endmodule