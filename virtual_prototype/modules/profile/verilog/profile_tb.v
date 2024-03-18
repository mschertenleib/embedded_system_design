/* set the time-units for simulation */
`timescale 1ps/1ps

module fifoTestbench;

  reg reset, clock;
  initial 
    begin
      reset = 1'b1;
      clock = 1'b0;                 /* set the initial values */
      repeat (4) #5 clock = ~clock; /* generate 2 clock periods */
      reset = 1'b0;                 /* de-activate the reset */
      forever #5 clock = ~clock;    /* generate a clock with a period of 10 time-units */
    end
  
  reg start_Sig, stall_Sig, busIdle_Sig, cpu_Cin_sig, done_out;
  reg [31:0] valueA_sig, valueB_Sig, result_out; 

  profileCi #(.customId(8'h00)) DUT
        (.start(start_Sig),
         .clock(clock),
         .reset(reset),
         .stall(stall_Sig),
         .busIdle(busIdle_Sig)
         .valueA(valueA_sig),
         .valueB(valueB_Sig),
         .ciN(cpu_Cin_sig),
         .done(done_out),
         .result(result_out));
  
  initial
    begin
      $dumpfile("fifoSignals.vcd"); /* define the name of the .vcd file that can be viewed by GTKWAVE */
      $dumpvars(1,DUT);             /* dump all signals inside the DUT-component in the .vcd file */
    end

  initial
    begin
      start_Sig = 1'b1;
      cpu_Cin_sig = 1'b1;
      stall_Sig = 1'b0;
      busIdle_Sig = 1'b1;
      @(negedge reset);            /* wait for the reset period to end */
      repeat(2) @(negedge clock);  /* wait for 2 clock cycles */
      valueA_sig[1:0] = 2'd0;      // look at counter0Value
      valueB_Sig[3:0] = 4'b0111 // enables
      valueB_Sig[7:4] = 4'b0000 // disables
      valueB_Sig[11:0]= 4'b0000 // resets
      repeat(32) @(negedge clock); /* wait for 32 clock cycles */
      valueA_sig[1:0] = 2'd3;      // look at counter3Value
      valueB_Sig[3:0] = 4'b1110 // enables
      repeat(32) @(negedge clock); /* wait for 32 clock cycles */
      valueB_Sig[11:0]= 4'b1111 // resets
      repeat(4) @(negedge clock); /* wait for 4 clock cycles */
      valueA_sig[1:0] = 2'd0;      // look at counter0Value
      valueB_Sig[3:0] = 4'b1111 // enables
      valueB_Sig[7:4] = 4'b0000 // disables
      valueB_Sig[11:0]= 4'b0000 // resets
      stall_Sig = 1'b1;
      busIdle_Sig = 1'b0;
      repeat(16) @(negedge clock); /* wait for 16 clock cycles */
      stall_Sig = 1'b0;
      busIdle_Sig = 1'b1;
      repeat(16) @(negedge clock); /* wait for 16 clock cycles */
      valueA_sig[1:0] = 2'd1; 
      repeat(4) @(negedge clock); /* wait for 4 clock cycles */
      valueA_sig[1:0] = 2'd2; 
      repeat(4) @(negedge clock); /* wait for 4 clock cycles */
      $finish;                     /* finish the simulation */
    end

endmodule
