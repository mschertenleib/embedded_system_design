

module kern_th_edge #(
    parameter thresh = 32
)(
    input wire [31:0] block_in,
    output wire thdx,thdy
);
    wire [8:0] mup;
  	wire [8:0] right;
  	wire [8:0] down;
    wire [8:0] mleft;
    assign mup[8:0] = {1'b1,~block_in[31:24]} + 1'b1;
    assign right[8:0] = {1'b0,block_in[23:16]};
    assign down[8:0] = {1'b0,block_in[15:8]};
    assign mleft[8:0] = {1'b1,~block_in[7:0]} + 1'b1;

    wire [8:0] dy = down + mup;     //simulation wizardry if use - operator
    wire [8:0] dx = right + mleft;
    wire sx = dx[8];
    wire sy = dy[8];

    // Thresholding, true if dx >= 32 or dx <= -33
    assign thdx = dx[3]^sx | dx[4]^sx | dx[5]^sx | dx[6]^sx | dx[7]^sx | dx[8]^sx;
    assign thdy = dy[3]^sy | dy[4]^sy | dy[5]^sy | dy[6]^sy | dy[7]^sy | dy[8]^sy;
endmodule

module edge_detect #(parameter stripwidth = 640)(
    //`define wire_adrressing
    `ifdef wire_adrressing
        input wire[7:0] strip [stripwidth*3],
    `else
        input wire[stripwidth*8*3:0] strip,
    `endif
    output wire[stripwidth-3:0] thdx_line,thdy_line
);
    genvar kernpos;
    generate
    for(kernpos = 1 ; kernpos < stripwidth-1 ; kernpos = kernpos + 1) begin: kernpos_loop
        `ifdef wire_adrressing
            wire[31:0] targpx= {  
                strip[kernpos],                 //px up
                strip[stripwidth+kernpos+1],    //px right
                strip[stripwidth*2 + kernpos],  //px down
                strip[stripwidth+kernpos-1]};   //px left
        `else
            wire[31:0] targpx= {    
                strip[kernpos*8+7:kernpos*8],                                           //px up
                strip[stripwidth + ((kernpos+1)*8)+7:stripwidth + ((kernpos+1)*8)],     //px right
                strip[(2*stripwidth) + (kernpos*8)+7:(2*stripwidth) + (kernpos*8)],     //px down
                strip[stripwidth + ((kernpos-1)*8)+7:stripwidth + ((kernpos-1)*8)]};    //px left
        `endif
        wire thdx,thdy;
        kern_th_edge #(
            .thresh(32)
        ) kern_th_edge_inst (
            .block_in(targpx),
            .thdx(thdx),
            .thdy(thdy)
        );
        assign thdx_line[kernpos-1] = thdx;
        assign thdy_line[kernpos-1] = thdy;
    end
    endgenerate

endmodule

module shift_strip #(parameter stripwidth = 640)(
    input wire[stripwidth*8*3:0] strip_in,
    output wire[stripwidth*8*3:0] strip_out
);
    assign strip_out[0:2*stripwidth-1] = strip_in[2*stripwidth:3*stripwidth-1];
    assign strip_out[2*stripwidth:3*stripwidth-1] = 0;
endmodule
