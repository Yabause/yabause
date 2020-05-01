#!/bin/bash

func_ABCD()
{
echo "#define init_"$2"_"$3"_"$4"_"$5>> sh2_functions.inc
echo "void FASTCALL call_"$2"_"$3"_"$4"_"$5"(SH2_struct* sh)" >> sh2_functions.inc
echo "{" >> sh2_functions.inc
echo "  "$1"(sh);" >> sh2_functions.inc
echo "}" >> sh2_functions.inc
}

func_ACD()
{
for b in {0..15}
do
echo "#define init_"$2"_"$b"_"$3"_"$4>> sh2_functions.inc
echo "void FASTCALL call_"$2"_"$b"_"$3"_"$4"(SH2_struct* sh)" >> sh2_functions.inc
echo "{" >> sh2_functions.inc
echo "  "$1"(sh,"$b");" >> sh2_functions.inc
echo "}" >> sh2_functions.inc
done
}

func_AD()
{
for b in {0..15}
do
for c in {0..15}
do
echo "#define init_"$2"_"$b"_"$c"_"$3>> sh2_functions.inc
echo "void FASTCALL call_"$2"_"$b"_"$c"_"$3"(SH2_struct* sh)" >> sh2_functions.inc
echo "{" >> sh2_functions.inc
echo "  "$1"(sh,"$b","$c");" >> sh2_functions.inc
echo "}" >> sh2_functions.inc
done
done
}

func_AB()
{
for c in {0..15}
do
for d in {0..15}
do
echo "#define init_"$2"_"$3"_"$c"_"$d>> sh2_functions.inc
echo "void FASTCALL call_"$2"_"$3"_"$c"_"$d"(SH2_struct* sh)" >> sh2_functions.inc
echo "{" >> sh2_functions.inc
echo "  "$1"(sh,("$c"<<4)+"$d");" >> sh2_functions.inc
echo "}" >> sh2_functions.inc
done
done
}

func_AB_n_d()
{
for c in {0..15}
do
for d in {0..15}
do
echo "#define init_"$2"_"$3"_"$c"_"$d>> sh2_functions.inc
echo "void FASTCALL call_"$2"_"$3"_"$c"_"$d"(SH2_struct* sh)" >> sh2_functions.inc
echo "{" >> sh2_functions.inc
echo "  "$1"(sh,"$c","$d");" >> sh2_functions.inc
echo "}" >> sh2_functions.inc
done
done
}

func_A()
{
for b in {0..15}
do
for c in {0..15}
do
for d in {0..15}
do
echo "#define init_"$2"_"$b"_"$c"_"$d>> sh2_functions.inc
echo "void FASTCALL call_"$2"_"$b"_"$c"_"$d"(SH2_struct* sh)" >> sh2_functions.inc
echo "{" >> sh2_functions.inc
echo "  "$1"(sh, "$b", ("$c"<<4)+"$d");" >> sh2_functions.inc
echo "}" >> sh2_functions.inc
done
done
done
}

func_A_NM_DISP()
{
for b in {0..15}
do
for c in {0..15}
do
for d in {0..15}
do
echo "#define init_"$2"_"$b"_"$c"_"$d>> sh2_functions.inc
echo "void FASTCALL call_"$2"_"$b"_"$c"_"$d"(SH2_struct* sh)" >> sh2_functions.inc
echo "{" >> sh2_functions.inc
echo "  "$1"(sh, "$b","$c", "$d");" >> sh2_functions.inc
echo "}" >> sh2_functions.inc
done
done
done
}

func_A_disp()
{
for b in {0..15}
do
for c in {0..15}
do
for d in {0..15}
do
echo "#define init_"$2"_"$b"_"$c"_"$d>> sh2_functions.inc
echo "void FASTCALL call_"$2"_"$b"_"$c"_"$d"(SH2_struct* sh)" >> sh2_functions.inc
echo "{" >> sh2_functions.inc
echo "  "$1"(sh, ("$b"<<8)+("$c"<<4)+"$d");" >> sh2_functions.inc
echo "}" >> sh2_functions.inc
done
done
done
}

func_default()
{
for a in {0..15}
do
for b in {0..15}
do
for c in {0..15}
do
for d in {0..15}
do
echo "#ifndef init_"$a"_"$b"_"$c"_"$d>> sh2_functions.inc
echo "#define init_"$a"_"$b"_"$c"_"$d>> sh2_functions.inc
echo "void FASTCALL call_"$a"_"$b"_"$c"_"$d"(SH2_struct* sh)" >> sh2_functions.inc
echo "{" >> sh2_functions.inc
echo "  SH2undecoded(sh);" >> sh2_functions.inc
echo "}" >> sh2_functions.inc
echo "#endif" >> sh2_functions.inc
done
done
done
done
}

func_opcode()
{
echo "opcode_func opcodeTable[0x10000] = { \\" >> sh2_functions.inc
for a in {0..15}
do
for b in {0..15}
do
for c in {0..15}
do
for d in {0..15}
do
echo "call_"$a"_"$b"_"$c"_"$d", \\" >> sh2_functions.inc
done
done
done
done
echo "};" >> sh2_functions.inc
}

echo "">sh2_functions.inc

func_ABCD SH2clrt 0 0 0 8
func_ABCD SH2nop 0 0 0 9
func_ABCD SH2rts 0 0 0 11
func_ACD SH2bsrf 0 0 3
func_ACD SH2braf 0 2 3
func_ABCD SH2clrmac  0  0  2  8
func_ABCD SH2div0u  0  0  1  9
func_ABCD SH2sleep  0  0  1  11
func_ACD SH2stspr  0  2  10
func_ACD SH2stsmacl  0  1  10
func_ACD SH2stsmach  0  0  10
func_ACD SH2stcvbr  0  2  2
func_ACD SH2stcsr  0  0  2
func_ACD SH2stcgbr  0  1  2
func_ABCD SH2sett  0  0  1  8
func_ABCD SH2rte  0  0  2  11
func_AD SH2mull  0  7
func_AD SH2movwl0  0  13
func_AD SH2movls0  0  6
func_ACD SH2movt  0  2  9
func_AD SH2movws0  0  5
func_AD SH2movll0  0  14
func_AD SH2movbs0  0  4
func_AD SH2movbl0  0  12
func_AD SH2macl  0  15
func_AD SH2tst  2  8
func_AD SH2xor  2  10
func_AD SH2xtrct  2  13
func_AB SH2xorm  12  14
func_AB SH2xori  12  10
func_AB SH2tstm  12  12
func_AB SH2tsti  12  8
func_AB SH2trapa  12  3
func_ACD SH2tas  4  1  11
func_AD SH2swapw  6  9
func_AD SH2swapb  6  8
func_AD SH2subv  3  11
func_AD SH2subc  3  10
func_AD SH2sub  3  8
func_ACD SH2stsmpr  4  2  2
func_ACD SH2stsmmacl  4  1  2
func_ACD SH2stsmmach  4  0  2
func_ACD SH2stcmvbr  4  2  3
func_ACD SH2stcmsr  4  0  3
func_ACD SH2stcmgbr  4  1  3
func_ACD SH2shlr16  4  2  9
func_ACD SH2shlr8  4  1  9
func_ACD SH2shlr2  4  0  9
func_ACD SH2shlr  4  0  1
func_ACD SH2shll16  4  2  8
func_ACD SH2shll8  4  1  8
func_ACD SH2shll2  4  0  8
func_ACD SH2shll  4  0  0
func_ACD SH2shar  4  2  1
func_ACD SH2shal  4  2  0
func_ACD SH2rotr  4  0  5
func_ACD SH2rotl  4  0  4
func_ACD SH2rotcr  4  2  5
func_ACD SH2rotcl  4  2  4
func_AB SH2orm  12  15
func_AB SH2ori  12  11
func_AD SH2or 2  11
func_AD SH2not  6  7
func_AD SH2negc  6  10
func_AD SH2neg  6  11
func_AD SH2mulu  2  14
func_AD SH2muls  2  15
func_AB SH2movwsg  12  1
func_AB_n_d SH2movws4  8  1
func_AD SH2movws  2  1
func_AD SH2movwp  6  5
func_AD SH2movwm  2  5
func_AB SH2movwlg  12  5
func_AB_n_d SH2movwl4  8  5
func_AD SH2movwl  6  1
func_A SH2movwi  9
func_AB SH2movlsg  12  2
func_A_NM_DISP SH2movls4  1
func_AD SH2movls  2  2
func_AD SH2movlp  6  6
func_AD SH2movlm  2  6
func_AB SH2movllg  12  6
func_A_NM_DISP SH2movll4  5
func_AD SH2movll  6  2
func_A SH2movli  13
func_A SH2movi  14
func_AB SH2movbsg  12  0
func_AB_n_d SH2movbs4  8  0
func_AD SH2movbs  2  0
func_AD SH2movbp  6  4
func_AD SH2movbm  2  4
func_AB SH2movblg  12  4
func_AB_n_d SH2movbl4  8  4
func_AD SH2movbl  6  0
func_AB SH2mova  12  7
func_AD SH2mov  6  3
func_AD SH2macw  4  15
func_ACD SH2ldspr  4  2  10
func_ACD SH2ldsmpr  4  2  6
func_ACD SH2ldsmmacl  4  1  6
func_ACD SH2ldsmmach  4  0  6
func_ACD SH2ldsmacl  4  1  10
func_ACD SH2ldsmach  4  0  10
func_ACD SH2ldcvbr  4  2  14
func_ACD SH2ldcsr  4  0  14
func_ACD SH2ldcmvbr  4  2  7
func_ACD SH2ldcmsr  4  0  7
func_ACD SH2ldcmgbr  4  1  7
func_ACD SH2ldcgbr  4  1  14
func_ACD SH2jsr  4  0  11
func_ACD SH2jmp  4  2  11
func_AD SH2extuw  6  13
func_AD SH2extub  6  12
func_AD SH2extsw  6  15
func_AD SH2extsb  6  14
func_ACD SH2dt  4  1  0
func_AD SH2dmulu  3  5
func_AD SH2dmuls  3  13
func_AD SH2div1  3  4
func_AD SH2div0s  2  7
func_AD SH2cmpstr  2  12
func_ACD SH2cmppz  4  1  1
func_ACD SH2cmppl  4  1  5
func_AB SH2cmpim  8  8
func_AD SH2cmphs  3  2
func_AD SH2cmphi  3  6
func_AD SH2cmpgt  3  7
func_AD SH2cmpge  3  3
func_AD SH2cmpeq  3  0
func_AB SH2bts  8  13
func_AB SH2bt  8  9
func_A_disp SH2bsr  11
func_A_disp SH2bra  10
func_AB SH2bfs  8  15
func_AB SH2bf  8  11
func_AB SH2andm  12  13
func_AB SH2andi  12  9
func_AD SH2and  2  9
func_AD SH2addv  3  15
func_AD SH2addc  3  14
func_A SH2addi  7
func_AD SH2add  3  12

func_default
func_opcode
