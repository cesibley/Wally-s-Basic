10 REM ==========================================================
20 REM DEF FN TORTURE TEST (GW-BASIC style) - v2
30 REM Fixes vs v1:
40 REM  - Correct NOT semantics (bitwise): NOT 5 = -6
50 REM  - Error harness distinguishes "error happened" vs "expected code"
60 REM ==========================================================

120 DEFINT A-Z
130 PASS=0: FAIL=0
140 TRIPPED_ANY=0
150 TRIPPED_MATCH=0
160 EXPECT_ERR=0: EXPECT_ERL=0
170 CALLDESC$=""
180 ON ERROR GOTO 9000

190 PRINT "Running DEF FN torture test..."
200 PRINT

210 GOSUB 1000
220 GOSUB 2000
230 GOSUB 3000
240 GOSUB 4000
250 GOSUB 5000

260 PRINT
270 PRINT "=================================="
280 PRINT "DEF FN TORTURE TEST COMPLETE"
290 PRINT "PASS=";PASS;" FAIL=";FAIL
300 PRINT "=================================="
310 END

900 REM ==========================================================
910 REM ASSERT NUMERIC
920 REM ==========================================================
930 IF GOT=EXPECT THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL:";DESC$;" EXPECT=";EXPECT;" GOT=";GOT
940 RETURN

950 REM ==========================================================
960 REM ASSERT STRING
970 REM ==========================================================
980 IF GOT$=EXPECT$ THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL:";DESC$;" EXPECT=[";EXPECT$;"] GOT=[";GOT$;"]"
990 RETURN

1000 REM ==========================================================
1010 REM TEST1: BASIC NUMERIC DEF FN
1020 REM ==========================================================
1030 PRINT "TEST1: Basic numeric DEF FN"

1040 DEF FNA(X)=X+1
1050 GOT=FNA(5): EXPECT=6: DESC$="FNA(5)=6": GOSUB 930
1060 GOT=FNA(-2): EXPECT=-1: DESC$="FNA(-2)=-1": GOSUB 930
1070 RETURN

2000 REM ==========================================================
2010 REM TEST2: MULTI-ARG + NESTED CALLS
2020 REM ==========================================================
2030 PRINT "TEST2: Multi-arg + nested calls"

2040 DEF FNB(X,Y)=X+Y*10
2050 GOT=FNB(2,3): EXPECT=32: DESC$="FNB(2,3)=32": GOSUB 930

2060 DEF FNC(X)=X*2
2070 GOT=FNC(FNA(4)): EXPECT=10: DESC$="FNC(FNA(4))=10": GOSUB 930
2080 RETURN

3000 REM ==========================================================
3010 REM TEST3: PRECEDENCE / NOT SEMANTICS INSIDE FN BODY
3020 REM ==========================================================
3030 PRINT "TEST3: Precedence + NOT semantics"

3040 DEF FNP(X)= -X^2
3050 GOT=FNP(3): EXPECT=-9: DESC$="FNP(3) = - (3^2) = -9": GOSUB 930

3060 DEF FNN(X)= NOT X
3070 GOT=FNN(0): EXPECT=-1: DESC$="NOT 0 = -1": GOSUB 930
3080 GOT=FNN(-1): EXPECT=0: DESC$="NOT -1 = 0": GOSUB 930
3090 GOT=FNN(5): EXPECT=-6: DESC$="NOT 5 = -6 (bitwise)": GOSUB 930

3100 DEF FNLOG(A,B,C)=A OR B AND C
3110 GOT=FNLOG(0,-1,0): EXPECT=0: DESC$="A OR (B AND C) precedence": GOSUB 930
3120 GOT=FNLOG(0,-1,-1): EXPECT=-1: DESC$="A OR (B AND C) precedence 2": GOSUB 930
3130 RETURN

4000 REM ==========================================================
4010 REM TEST4: STRING DEF FN$
4020 REM ==========================================================
4030 PRINT "TEST4: String DEF FN$"

4040 DEF FNS$(A$)=A$+"!"
4050 GOT$=FNS$("HI"): EXPECT$="HI!": DESC$="FNS$(" + CHR$(34) + "HI" + CHR$(34) + ")": GOSUB 950

4060 DEF FN2$(A$,B$)=A$+":"+B$
4070 GOT$=FN2$("A","B"): EXPECT$="A:B": DESC$="FN2$(" + CHR$(34) + "A" + CHR$(34) + "," + CHR$(34) + "B" + CHR$(34) + ")": GOSUB 950
4080 RETURN

5000 REM ==========================================================
5010 REM TEST5: ERROR CASES
5020 REM ==========================================================
5030 PRINT "TEST5: Error cases"

5040 REM (A) Undefined FN call should error (GW-BASIC: "Undefined user function")
5050 EXPECT_ERR=35: EXPECT_ERL=5080
5060 TRIPPED_ANY=0: TRIPPED_MATCH=0
5070 CALLDESC$="Undefined FN should error"
5080 GOT=FNU(1)
5090 IF TRIPPED_ANY=0 THEN FAIL=FAIL+1: PRINT "FAIL:";CALLDESC$;" (no error)" ELSE PASS=PASS+1
5100 IF TRIPPED_MATCH=0 THEN FAIL=FAIL+1: PRINT "FAIL:";CALLDESC$;" (wrong ERR/ERL)"

5110 REM (B) Wrong arg count should error (often ERR=5 in GW-BASIC)
5120 DEF FND(X,Y)=X-Y
5130 EXPECT_ERR=5: EXPECT_ERL=5150
5140 TRIPPED_ANY=0: TRIPPED_MATCH=0
5145 CALLDESC$="Wrong arg count should error"
5150 GOT=FND(1)
5160 IF TRIPPED_ANY=0 THEN FAIL=FAIL+1: PRINT "FAIL:";CALLDESC$;" (no error)" ELSE PASS=PASS+1
5170 IF TRIPPED_MATCH=0 THEN FAIL=FAIL+1: PRINT "FAIL:";CALLDESC$;" (wrong ERR/ERL)"

5180 REM (C) Type mismatch: numeric FN called with string should error (ERR=13)
5190 DEF FNE(X)=X+1
5200 EXPECT_ERR=13: EXPECT_ERL=5220
5210 TRIPPED_ANY=0: TRIPPED_MATCH=0
5215 CALLDESC$="Type mismatch numeric FN(string) should error"
5220 GOT=FNE("A")
5230 IF TRIPPED_ANY=0 THEN FAIL=FAIL+1: PRINT "FAIL:";CALLDESC$;" (no error)" ELSE PASS=PASS+1
5240 IF TRIPPED_MATCH=0 THEN FAIL=FAIL+1: PRINT "FAIL:";CALLDESC$;" (wrong ERR/ERL)"

5250 REM (D) Type mismatch: string FN$ called with numeric should error (ERR=13)
5260 DEF FNF$(A$)=A$
5270 EXPECT_ERR=13: EXPECT_ERL=5290
5280 TRIPPED_ANY=0: TRIPPED_MATCH=0
5285 CALLDESC$="Type mismatch string FN$(numeric) should error"
5290 GOT$=FNF$(5)
5300 IF TRIPPED_ANY=0 THEN FAIL=FAIL+1: PRINT "FAIL:";CALLDESC$;" (no error)" ELSE PASS=PASS+1
5310 IF TRIPPED_MATCH=0 THEN FAIL=FAIL+1: PRINT "FAIL:";CALLDESC$;" (wrong ERR/ERL)"

5320 RETURN

9000 REM ==========================================================
9010 REM ERROR HANDLER
9020 REM ==========================================================
9030 TRIPPED_ANY=1
9040 IF ERR=EXPECT_ERR AND ERL=EXPECT_ERL THEN TRIPPED_MATCH=1

9050 IF TRIPPED_MATCH=0 THEN
9060   FAIL=FAIL+1
9070   PRINT "FAIL:";CALLDESC$;" expected ERR=";EXPECT_ERR;" ERL=";EXPECT_ERL;" got ERR=";ERR;" ERL=";ERL
9080 END IF

9090 ERR=0
9100 RESUME NEXT
