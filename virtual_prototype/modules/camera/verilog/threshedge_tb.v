
module threshedge_tb;

    // Inputs
    reg [7:0] up, down, left, right;
    wire [31:0] din = {up,right,down,left};
    
    // Outputs
    wire thx,thy;
    
    // Instantiate the module under test
    kern_th_edge dut (
        .block_in(din),
        .thdx(thx),
        .thdy(thy)
    );

    task automatic test_th;
        input [7:0] test_up, test_down, test_left, test_right;
        
        begin
            up = test_up;
            down = test_down;
            left = test_left;
            right = test_right;
            #1
            $display(
                "IN: r-l = %d-%d, d-u = %d-%d \nOUT: thx = %b, thy = %b \nEXP: thx = %b, thy = %b \n",
                right,left,down,up,
                thx,thy,
                ((-right+left)>32),((down-up)>=32)
            );
        end
    endtask
    
    initial begin
        for (integer i = 31; i <= 33 ; i = i+1) begin
            $display("###### i = %d ######",i);
            test_th(0,i,i,0);
        end

        test_th(0,0,0,0);
        test_th(255,0,0,255);
        

        #10 $display("Testbench finished");
        $finish;
    end

endmodule