module TP5(input logic clk,
           input logic T3,
           input logic init,
           input logic [3:0] a,
           output logic [3:0] b);

logic [3:0] c;

always_ff@(posedge clk)
    begin
        if (init) begin
            c <= a;
            b <= ~a;
        end
        if (T3) begin
            c <= b;
            b <= c;
        end
    end
	 endmodule

//flop R1(clk, T3, init, a, b, c);
//flop R2(clk, T3, init, ~a, c, b);