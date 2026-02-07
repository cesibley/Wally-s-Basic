10 REM ==========================================================
20 REM EXPRESSION / PRECEDENCE TORTURE TEST (GW-BASIC)
30 REM Notes (GW-BASIC semantics):
40 REM  - NOT is bitwise complement on integer-rounded operand.
50 REM  - Relational results are TRUE=-1, FALSE=0.
60 REM  - Precedence (subset): ^, unary +/- , * / MOD, + -, relops, NOT, AND, OR, XOR
70 REM ==========================================================
80 DEFINT A-Z
90 PASS=0: FAIL=0
100 PRINT "Running EXPRESSION torture test..."
110 PRINT
120 GOSUB 1000
130 GOSUB 2000
140 GOSUB 3000
150 GOSUB 4000
160 GOSUB 5000
170 PRINT
180 PRINT "=================================="
190 PRINT "EXPRESSION TORTURE TEST COMPLETE"
200 PRINT "PASS=";PASS;" FAIL=";FAIL
210 PRINT "=================================="
220 END
900 REM ==========================================================
910 REM ASSERT
920 REM ==========================================================
930 IF GOT=EXPECT THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL:";DESC$;" EXPECT=";EXPECT;" GOT=";GOT
940 RETURN
1000 REM ==========================================================
1010 REM UNARY MINUS VS EXPONENT
1020 REM ==========================================================
1030 PRINT "TEST1: Unary minus vs exponent"
1040 GOT=-2^2: EXPECT=-4: DESC$="-2^2 = -4": GOSUB 900
1050 GOT=(-2)^2: EXPECT=4: DESC$="(-2)^2 = 4": GOSUB 900
1060 GOT=-(2^2): EXPECT=-4: DESC$="-(2^2) = -4": GOSUB 900
1070 RETURN
2000 REM ==========================================================
2010 REM NOT (BITWISE) SEMANTICS + PRECEDENCE VS +
2020 REM ==========================================================
2030 PRINT "TEST2: NOT semantics / precedence"
2040 GOT=NOT 0: EXPECT=-1: DESC$="NOT 0 = -1": GOSUB 900
2050 GOT=NOT 5: EXPECT=-6: DESC$="NOT 5 = -6": GOSUB 900
2060 GOT=NOT 1+1: EXPECT=-3: DESC$="NOT 1+1 = NOT(1+1) = -3": GOSUB 900
2070 GOT=NOT (1+1): EXPECT=-3: DESC$="NOT (1+1) = -3": GOSUB 900
2080 RETURN
3000 REM ==========================================================
3010 REM AND / OR PRECEDENCE (BITWISE)
3020 REM ==========================================================
3030 PRINT "TEST3: AND / OR precedence"
3040 GOT=1 OR 0 AND 0: EXPECT=1: DESC$="1 OR 0 AND 0 = 1": GOSUB 900
3050 GOT=(1 OR 0) AND 0: EXPECT=0: DESC$="(1 OR 0) AND 0 = 0": GOSUB 900
3060 GOT=1 OR (0 AND 0): EXPECT=1: DESC$="1 OR (0 AND 0) = 1": GOSUB 900
3070 RETURN
4000 REM ==========================================================
4010 REM BITWISE AND / OR / XOR
4020 REM ==========================================================
4030 PRINT "TEST4: Bitwise operators"
4040 GOT=5 AND 3: EXPECT=1: DESC$="5 AND 3 = 1": GOSUB 900
4050 GOT=5 OR 2: EXPECT=7: DESC$="5 OR 2 = 7": GOSUB 900
4060 GOT=5 XOR 1: EXPECT=4: DESC$="5 XOR 1 = 4": GOSUB 900
4070 RETURN
5000 REM ==========================================================
5010 REM MIXED EXPRESSIONS (RELATIONAL TRUTH + BITWISE)
5020 REM ==========================================================
5030 PRINT "TEST5: Mixed expressions"
5040 GOT=(2+3)*4=20: EXPECT=-1: DESC$="(2+3)*4=20 is TRUE (-1)": GOSUB 900
5050 GOT=(2+3)*4=21: EXPECT=0: DESC$="(2+3)*4=21 is FALSE (0)": GOSUB 900
5060 GOT=(5>3) AND (2>1): EXPECT=-1: DESC$="(5>3) AND (2>1) = -1": GOSUB 900
5070 GOT=(5>3) AND (1>2): EXPECT=0: DESC$="(5>3) AND (1>2) = 0": GOSUB 900
5080 GOT=(5>3) OR (1>2): EXPECT=-1: DESC$="(5>3) OR (1>2) = -1": GOSUB 900
5090 GOT=(5>3) XOR (5>3): EXPECT=0: DESC$="TRUE XOR TRUE = 0": GOSUB 900
5100 RETURN
