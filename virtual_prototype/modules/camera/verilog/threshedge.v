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
    assign thdx = dx[5]^sx | dx[6]^sx | dx[7]^sx | dx[8]^sx;
    assign thdy = dy[5]^sy | dy[6]^sy | dy[7]^sy | dy[8]^sy;
endmodule

module conv_line #(parameter stripwidth = 640)(
    input wire update,
    input wire[7:0] strip [stripwidth*3],
    output wire[stripwidth-3:0] thdx_line,thdy_line
);
    for(genvar kernpos = 1 ; kernpos < stripwidth-1 ; kernpos = kernpos + 1) begin
        wire[31:0] targpx= {  strip[kernpos],           //px up
                        strip[stripwidth+kernpos+1],    //px right
                        strip[stripwidth*2 + kernpos], //px down
                        strip[stripwidth+kernpos-1]};    //px left
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


endmodule
