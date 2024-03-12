module profileCi #( parameter[7:0] customId = 8'h00 )
		  ( input wire start,
			  clock,
			  reset,
			  stall,
			  busIdle,
		input wire[31:0] valueA,
		valueB,
		input wire[7:0] ciN,
		output wire done,
		output wire[31:0] result );

  wire counter0Enable;
  wire [31:0] counter0Value;
  counter #( .WIDTH(32) ) counter0
           ( .reset(reset),
	.clock(clock),
        .enable(s_counter0_value),
	.direction(1'b1)),
	.counterValue(_counter0_value);
 
	always @(posedge clock) begin
		if (ciN == customId && start == 1'b1) begin
			result = (valueA[1:0] == 2'd0 ? counter0Value)
		end else begin
			result = 32'd0;
			done = 1'b0;
		end
	end
endmodule

