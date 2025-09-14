module top(
    input clk,
    input rst,
    input [7:0] sw,
    output [15:0] ledr
);

wire ledon;
switch s(sw[0], sw[1], ledon);

led my_led(
    .clk(clk),
    .rst(rst),
    .sw({8{ledon}}),
    .ledr(ledr)
);

endmodule