// tech mapping module that converts auto-generated \$lut cells to valid
// liberty cells
//
// Unlike map_lut_to_lutmux.ys, this techmapping ensures all output cells have
// the same bit-width, which is required for parallelism in some FHE backends,
// i.e., the truth table sizes and input dimensions must all be the same.
// Since bootstrap has the same latency regardless of the LUT size, it's not
// problem to pad with zeros.

(* techmap_celltype = "\$lut" *)
module _tjc_lut4 (A, Y);
    parameter LUT = 16'h0;
    parameter WIDTH = 4;
    input [WIDTH-1:0] A;
    output Y;

    wire _TECHMAP_FAIL_ = (WIDTH != 4);

    wire [2**WIDTH-1:0] PBITS = LUT;

    (* LUT=LUT *)
    lut4 _TECHMAP_REPLACE_(.D(A[3]), .C(A[2]), .B(A[1]), .A(A[0]),
            .P15(PBITS[15]),
            .P14(PBITS[14]),
            .P13(PBITS[13]),
            .P12(PBITS[12]),
            .P11(PBITS[11]),
            .P10(PBITS[10]),
            .P9(PBITS[9]),
            .P8(PBITS[8]),
            .P7(PBITS[7]),
            .P6(PBITS[6]),
            .P5(PBITS[5]),
            .P4(PBITS[4]),
            .P3(PBITS[3]),
            .P2(PBITS[2]),
            .P1(PBITS[1]),
            .P0(PBITS[0]),
            .Y(Y));
endmodule

(* techmap_celltype = "\$lut" *)
module _tjc_lut3 (A, Y);
    parameter LUT = 8'h0;
    parameter WIDTH = 3;
    input [WIDTH-1:0] A;
    output Y;

    wire _TECHMAP_FAIL_ = (WIDTH != 3);

    wire [2**WIDTH-1:0] PBITS = LUT;

    (* LUT=LUT *)
    lut4 _TECHMAP_REPLACE_(.D(0), .C(A[2]), .B(A[1]), .A(A[0]),
            .P15(0),
            .P14(0),
            .P13(0),
            .P12(0),
            .P11(0),
            .P10(0),
            .P9(0),
            .P8(0),
            .P7(PBITS[7]),
            .P6(PBITS[6]),
            .P5(PBITS[5]),
            .P4(PBITS[4]),
            .P3(PBITS[3]),
            .P2(PBITS[2]),
            .P1(PBITS[1]),
            .P0(PBITS[0]),
            .Y(Y));
endmodule

(* techmap_celltype = "\$lut" *)
module _tjc_lut2 (A, Y);
    parameter LUT = 4'h0;
    parameter WIDTH = 2;
    input [WIDTH-1:0] A;
    output Y;

    wire _TECHMAP_FAIL_ = (WIDTH != 2);

    wire [2**WIDTH-1:0] PBITS = LUT;

    (* LUT=LUT *)
    lut4 _TECHMAP_REPLACE_(.D(0), .C(0), .B(A[1]), .A(A[0]),
            .P15(0),
            .P14(0),
            .P13(0),
            .P12(0),
            .P11(0),
            .P10(0),
            .P9(0),
            .P8(0),
            .P7(0),
            .P6(0),
            .P5(0),
            .P4(0),
            .P3(PBITS[3]),
            .P2(PBITS[2]),
            .P1(PBITS[1]),
            .P0(PBITS[0]),
            .Y(Y));
endmodule

(* techmap_celltype = "\$lut" *)
module _tjc_lut1 (A, Y);
    parameter LUT = 2'h0;
    parameter WIDTH = 1;
    input [WIDTH-1:0] A;
    output Y;

    wire _TECHMAP_FAIL_ = (WIDTH != 1);

    wire [2**WIDTH-1:0] PBITS = LUT;

    (* LUT=LUT *)
    lut4 _TECHMAP_REPLACE_(.D(0), .C(0), .B(0), .A(A[0]),
            .P15(0),
            .P14(0),
            .P13(0),
            .P12(0),
            .P11(0),
            .P10(0),
            .P9(0),
            .P8(0),
            .P7(0),
            .P6(0),
            .P5(0),
            .P4(0),
            .P3(0),
            .P2(0),
            .P1(PBITS[1]),
            .P0(PBITS[0]),
            .Y(Y));
endmodule

