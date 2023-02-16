// tech mapping module that converts auto-generated \$lut cells to valid
// liberty cells
//
// Unlike map_lut_to_lutmux.ys, this techmapping ensures all output cells have
// the same bit-width, which is required for parallelism in some FHE backends,
// i.e., the truth table sizes and input dimensions must all be the same.
// Since bootstrap has the same latency regardless of the LUT size, it's not
// problem to pad with zeros.

(* techmap_celltype = "\$lut" *)
module _tjc_lut6 (A, Y);
    parameter LUT = 64'h0;
    parameter WIDTH = 6;
    input [WIDTH-1:0] A;
    output Y;

    wire _TECHMAP_FAIL_ = (WIDTH != 6);

    wire [2**WIDTH-1:0] PBITS = LUT;

    (* LUT=LUT *)
    lut6 _TECHMAP_REPLACE_(.F(A[5]), .E(A[4]), .D(A[3]), .C(A[2]), .B(A[1]), .A(A[0]),
            .P63(PBITS[63]),
            .P62(PBITS[62]),
            .P61(PBITS[61]),
            .P60(PBITS[60]),
            .P59(PBITS[59]),
            .P58(PBITS[58]),
            .P57(PBITS[57]),
            .P56(PBITS[56]),
            .P55(PBITS[55]),
            .P54(PBITS[54]),
            .P53(PBITS[53]),
            .P52(PBITS[52]),
            .P51(PBITS[51]),
            .P50(PBITS[50]),
            .P49(PBITS[49]),
            .P48(PBITS[48]),
            .P47(PBITS[47]),
            .P46(PBITS[46]),
            .P45(PBITS[45]),
            .P44(PBITS[44]),
            .P43(PBITS[43]),
            .P42(PBITS[42]),
            .P41(PBITS[41]),
            .P40(PBITS[40]),
            .P39(PBITS[39]),
            .P38(PBITS[38]),
            .P37(PBITS[37]),
            .P36(PBITS[36]),
            .P35(PBITS[35]),
            .P34(PBITS[34]),
            .P33(PBITS[33]),
            .P32(PBITS[32]),
            .P31(PBITS[31]),
            .P30(PBITS[30]),
            .P29(PBITS[29]),
            .P28(PBITS[28]),
            .P27(PBITS[27]),
            .P26(PBITS[26]),
            .P25(PBITS[25]),
            .P24(PBITS[24]),
            .P23(PBITS[23]),
            .P22(PBITS[22]),
            .P21(PBITS[21]),
            .P20(PBITS[20]),
            .P19(PBITS[19]),
            .P18(PBITS[18]),
            .P17(PBITS[17]),
            .P16(PBITS[16]),
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
module _tjc_lut5 (A, Y);
    parameter LUT = 32'h0;
    parameter WIDTH = 5;
    input [WIDTH-1:0] A;
    output Y;

    wire _TECHMAP_FAIL_ = (WIDTH != 5);

    wire [2**WIDTH-1:0] PBITS = LUT;

    (* LUT=LUT *)
    lut6 _TECHMAP_REPLACE_(.F(0), .E(A[4]), .D(A[3]), .C(A[2]), .B(A[1]), .A(A[0]),
            .P63(0),
            .P62(0),
            .P61(0),
            .P60(0),
            .P59(0),
            .P58(0),
            .P57(0),
            .P56(0),
            .P55(0),
            .P54(0),
            .P53(0),
            .P52(0),
            .P51(0),
            .P50(0),
            .P49(0),
            .P48(0),
            .P47(0),
            .P46(0),
            .P45(0),
            .P44(0),
            .P43(0),
            .P42(0),
            .P41(0),
            .P40(0),
            .P39(0),
            .P38(0),
            .P37(0),
            .P36(0),
            .P35(0),
            .P34(0),
            .P33(0),
            .P32(0),
            .P31(PBITS[31]),
            .P30(PBITS[30]),
            .P29(PBITS[29]),
            .P28(PBITS[28]),
            .P27(PBITS[27]),
            .P26(PBITS[26]),
            .P25(PBITS[25]),
            .P24(PBITS[24]),
            .P23(PBITS[23]),
            .P22(PBITS[22]),
            .P21(PBITS[21]),
            .P20(PBITS[20]),
            .P19(PBITS[19]),
            .P18(PBITS[18]),
            .P17(PBITS[17]),
            .P16(PBITS[16]),
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
module _tjc_lut4 (A, Y);
    parameter LUT = 16'h0;
    parameter WIDTH = 4;
    input [WIDTH-1:0] A;
    output Y;

    wire _TECHMAP_FAIL_ = (WIDTH != 4);

    wire [2**WIDTH-1:0] PBITS = LUT;

    (* LUT=LUT *)
    lut6 _TECHMAP_REPLACE_(.F(0), .E(0), .D(A[3]), .C(A[2]), .B(A[1]), .A(A[0]),
            .P63(0),
            .P62(0),
            .P61(0),
            .P60(0),
            .P59(0),
            .P58(0),
            .P57(0),
            .P56(0),
            .P55(0),
            .P54(0),
            .P53(0),
            .P52(0),
            .P51(0),
            .P50(0),
            .P49(0),
            .P48(0),
            .P47(0),
            .P46(0),
            .P45(0),
            .P44(0),
            .P43(0),
            .P42(0),
            .P41(0),
            .P40(0),
            .P39(0),
            .P38(0),
            .P37(0),
            .P36(0),
            .P35(0),
            .P34(0),
            .P33(0),
            .P32(0),
            .P31(0),
            .P30(0),
            .P29(0),
            .P28(0),
            .P27(0),
            .P26(0),
            .P25(0),
            .P24(0),
            .P23(0),
            .P22(0),
            .P21(0),
            .P20(0),
            .P19(0),
            .P18(0),
            .P17(0),
            .P16(0),
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
    lut6 _TECHMAP_REPLACE_(.F(0), .E(0), .D(0), .C(A[2]), .B(A[1]), .A(A[0]),
            .P63(0),
            .P62(0),
            .P61(0),
            .P60(0),
            .P59(0),
            .P58(0),
            .P57(0),
            .P56(0),
            .P55(0),
            .P54(0),
            .P53(0),
            .P52(0),
            .P51(0),
            .P50(0),
            .P49(0),
            .P48(0),
            .P47(0),
            .P46(0),
            .P45(0),
            .P44(0),
            .P43(0),
            .P42(0),
            .P41(0),
            .P40(0),
            .P39(0),
            .P38(0),
            .P37(0),
            .P36(0),
            .P35(0),
            .P34(0),
            .P33(0),
            .P32(0),
            .P31(0),
            .P30(0),
            .P29(0),
            .P28(0),
            .P27(0),
            .P26(0),
            .P25(0),
            .P24(0),
            .P23(0),
            .P22(0),
            .P21(0),
            .P20(0),
            .P19(0),
            .P18(0),
            .P17(0),
            .P16(0),
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
    lut6 _TECHMAP_REPLACE_(.F(0), .E(0), .D(0), .C(0), .B(A[1]), .A(A[0]),
            .P63(0),
            .P62(0),
            .P61(0),
            .P60(0),
            .P59(0),
            .P58(0),
            .P57(0),
            .P56(0),
            .P55(0),
            .P54(0),
            .P53(0),
            .P52(0),
            .P51(0),
            .P50(0),
            .P49(0),
            .P48(0),
            .P47(0),
            .P46(0),
            .P45(0),
            .P44(0),
            .P43(0),
            .P42(0),
            .P41(0),
            .P40(0),
            .P39(0),
            .P38(0),
            .P37(0),
            .P36(0),
            .P35(0),
            .P34(0),
            .P33(0),
            .P32(0),
            .P31(0),
            .P30(0),
            .P29(0),
            .P28(0),
            .P27(0),
            .P26(0),
            .P25(0),
            .P24(0),
            .P23(0),
            .P22(0),
            .P21(0),
            .P20(0),
            .P19(0),
            .P18(0),
            .P17(0),
            .P16(0),
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
    lut6 _TECHMAP_REPLACE_(.F(0), .E(0), .D(0), .C(0), .B(0), .A(A[0]),
            .P63(0),
            .P62(0),
            .P61(0),
            .P60(0),
            .P59(0),
            .P58(0),
            .P57(0),
            .P56(0),
            .P55(0),
            .P54(0),
            .P53(0),
            .P52(0),
            .P51(0),
            .P50(0),
            .P49(0),
            .P48(0),
            .P47(0),
            .P46(0),
            .P45(0),
            .P44(0),
            .P43(0),
            .P42(0),
            .P41(0),
            .P40(0),
            .P39(0),
            .P38(0),
            .P37(0),
            .P36(0),
            .P35(0),
            .P34(0),
            .P33(0),
            .P32(0),
            .P31(0),
            .P30(0),
            .P29(0),
            .P28(0),
            .P27(0),
            .P26(0),
            .P25(0),
            .P24(0),
            .P23(0),
            .P22(0),
            .P21(0),
            .P20(0),
            .P19(0),
            .P18(0),
            .P17(0),
            .P16(0),
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
