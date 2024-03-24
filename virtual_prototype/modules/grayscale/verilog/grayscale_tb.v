/* set the time-units for simulation */
`timescale 1ps/1ps

module rgb2gray_tb;

  reg reset, clock;
  initial 
    begin
      reset = 1'b1;
      clock = 1'b0;                 /* set the initial values */
      repeat (4) #5 clock = ~clock; /* generate 2 clock periods */
      reset = 1'b0;                 /* de-activate the reset */
      forever #5 clock = ~clock;    /* generate a clock with a period of 10 time-units */
    end
  
  reg start_Sig;
  reg [7:0] cpu_Cin_sig;
  reg [31:0] valueA_sig, valueB_Sig;
  wire done_out;
  wire [31:0] result_out; 
  integer i;
  integer seed = 1;

  rgb565GrayscaleIse #(.customInstructionId(8'h00)) DUT
        (.start(start_Sig),
         .valueA(valueA_sig),
         .valueB(valueB_Sig),
         .iseId(cpu_Cin_sig),
         .done(done_out),
         .result(result_out));
  
  initial
    begin
      $dumpfile("rgb2gray_Signals.vcd"); /* define the name of the .vcd file that can be viewed by GTKWAVE */
      $dumpvars(1,DUT);             /* dump all signals inside the DUT-component in the .vcd file */
    end

  initial
    begin
      valueA_sig = 32'd0;
      valueB_Sig = 32'd0;
      cpu_Cin_sig = 8'd0;
      start_Sig = 1'b1;

      @(negedge reset);            /* wait for the reset period to end */
      repeat(2) @(negedge clock);  /* wait for 2 clock cycles */
    
      for(i = 0; i < 2**5; i = i + 1)begin
        valueA_sig[15:11] = i;
        repeat(1) @(negedge clock);
      end
      valueA_sig[15:11] = 0;
      start_Sig = 1'b0;
      for(i = 0; i < 2**6; i = i + 1)begin
        valueA_sig[10:5] = i;
        repeat(1) @(negedge clock);
      end
      valueA_sig[10:5] = 0;
      for(i = 0; i < 2**5; i = i + 1)begin
        valueA_sig[4:0] = i;
        repeat(1) @(negedge clock);
      end
      valueA_sig[15:0] = 16'hffff;
      repeat(1) @(negedge clock);
      $finish;                     /* finish the simulation */
    end

endmodule
