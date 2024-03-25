/* set the time-units for simulation */
`timescale 1ps/1ps

module profileTestbench;

  reg reset, clock;
  initial 
    begin
      reset = 1'b1;
      clock = 1'b0;                 /* set the initial values */
      repeat (4) #5 clock = ~clock; /* generate 2 clock periods */
      reset = 1'b0;                 /* de-activate the reset */
      forever #5 clock = ~clock;    /* generate a clock with a period of 10 time-units */
    end
  
  reg start_Sig, stall_Sig, busIdle_Sig;
  reg [7:0] cpu_Cin_sig;
  reg [31:0] valueA_sig, valueB_Sig;
  wire done_out;
  wire [31:0] result_out; 
  integer i;
  integer seed = 1;

  profileCi #(.customId(8'h00)) DUT
        (.start(start_Sig),
         .clock(clock),
         .reset(reset),
         .stall(stall_Sig),
         .busIdle(busIdle_Sig),
         .valueA(valueA_sig),
         .valueB(valueB_Sig),
         .ciN(cpu_Cin_sig),
         .done(done_out),
         .result(result_out));
  
  initial
    begin
      $dumpfile("Profile_Signals.vcd"); /* define the name of the .vcd file that can be viewed by GTKWAVE */
      $dumpvars(1,DUT);             /* dump all signals inside the DUT-component in the .vcd file */
    end

  initial
    begin
      valueA_sig = 32'd0;
      valueB_Sig = 32'd0;
      cpu_Cin_sig = 8'd0;
      start_Sig = 1'b1;
      stall_Sig = 1'b0;
      busIdle_Sig = 1'b1;

      @(negedge reset);            /* wait for the reset period to end */
      repeat(2) @(negedge clock);  /* wait for 2 clock cycles */
      valueA_sig[1:0] = 2'd0;      // look at counter0Value
      valueB_Sig[3:0] = 4'b0101; // enables
      valueB_Sig[7:4] = 4'b0000; // disables
      valueB_Sig[11:8]= 4'b0000; // resets
      stall_Sig = 1'b0;
      busIdle_Sig = 1'b1;
      repeat(2) @(negedge clock);
      valueB_Sig = 32'd0;

      repeat(10) @(negedge clock); 
      valueA_sig[1:0] = 2'd1;      // look at counter0Value
      valueB_Sig[3:0] = 4'b0010; // enables
      valueB_Sig[7:4] = 4'b0001; // disables
      valueB_Sig[11:8]= 4'b0001; // resets
      stall_Sig = 1'b1;
      busIdle_Sig = 1'b0;
      repeat(2) @(negedge clock);
      valueB_Sig = 32'd0;

      repeat(10) @(negedge clock); 
      valueA_sig[1:0] = 2'd2;      // look at counter0Value
      valueB_Sig[3:0] = 4'b0100; // enables
      valueB_Sig[7:4] = 4'b0010; // disables
      valueB_Sig[11:8]= 4'b0010; // resets
      stall_Sig = 1'b0;
      busIdle_Sig = 1'b1;
      repeat(2) @(negedge clock);
      valueB_Sig = 32'd0;

      repeat(10) @(negedge clock); 
      valueA_sig[1:0] = 2'd3;      // look at counter0Value
      valueB_Sig[3:0] = 4'b1110; // enables
      valueB_Sig[7:4] = 4'b0000; // disables
      valueB_Sig[11:8]= 4'b0100; // resets
      stall_Sig = 1'b0;
      busIdle_Sig = 1'b1;
      repeat(2) @(negedge clock);
      valueB_Sig = 32'd0;
      repeat(10) @(negedge clock);
      valueA_sig[1:0] = 2'd0;
      valueB_Sig[11:0] = 12'b111110001111;  
      repeat(2) @(negedge clock);
      valueB_Sig = 32'd0;

      for(i = 0; i < 100; i = i + 1)begin
        busIdle_Sig = $random(seed);
        stall_Sig = $random(seed);
        cpu_Cin_sig = (i%9)%4;
        $display("%0t %0d",stallCounterValue/counterValue)
        repeat(1) @(negedge clock);
      end
      $finish;                     /* finish the simulation */
    end

endmodule
