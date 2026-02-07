10 REM ==========================================================
20 REM GOSUB / RETURN TORTURE TEST (GW-BASIC style)
30 REM Tests:
40 REM  - Basic GOSUB / RETURN
50 REM  - Nested GOSUBs
60 REM  - Conditional RETURN paths
70 REM  - RETURN without GOSUB (must error)
80 REM ==========================================================

100 DEFINT A-Z
110 PASS=0: FAIL=0
120 TRIPPED=0
130 EXPECT_ERR=0: EXPECT_ERL=0
140 ON ERROR GOTO 8000

150 PRINT "Running GOSUB / RETURN torture test..."
160 PRINT

170 GOSUB 1000
180 GOSUB 2000
190 GOSUB 3000

195 GOTO 4000

900 REM ==========================================================
910 REM ASSERT NUMERIC
920 REM ==========================================================
930 IF GOT=EXPECT THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL:";DESC$;" EXPECT=";EXPECT;" GOT=";GOT
940 RETURN

1000 REM ==========================================================
1010 REM TEST1: BASIC GOSUB / RETURN
1020 REM ==========================================================
1030 PRINT "TEST1: Basic GOSUB / RETURN"

1040 A=0
1050 GOSUB 1100
1060 GOT=A: EXPECT=1: DESC$="Single GOSUB increments A": GOSUB 930
1070 RETURN

1100 A=A+1
1110 RETURN

2000 REM ==========================================================
2010 REM TEST2: NESTED GOSUBS
2020 REM ==========================================================
2030 PRINT "TEST2: Nested GOSUBs"

2040 A=0
2050 GOSUB 2100
2060 GOT=A: EXPECT=3: DESC$="Nested GOSUB chain returns correctly": GOSUB 930
2070 RETURN

2100 A=A+1
2110 GOSUB 2200
2120 A=A+1
2130 RETURN

2200 A=A+1
2210 RETURN

3000 REM ==========================================================
3010 REM TEST3: CONDITIONAL RETURN PATHS
3020 REM ==========================================================
3030 PRINT "TEST3: Conditional RETURN paths"

3040 A=0
3050 X=1
3060 GOSUB 3300
3070 GOT=A: EXPECT=1: DESC$="Early RETURN executed": GOSUB 930

3080 A=0
3090 X=0
3100 GOSUB 3300
3110 GOT=A: EXPECT=2: DESC$="Late RETURN executed": GOSUB 930
3120 RETURN

3300 IF X THEN A=A+1: RETURN
3310 A=A+1
3320 A=A+1
3330 RETURN

4000 REM ==========================================================
4010 REM TEST4: RETURN WITHOUT GOSUB (must error)
4020 REM IMPORTANT: This test must be in MAINLINE (not in a GOSUB).
4030 REM ==========================================================
4040 PRINT "TEST4: RETURN without GOSUB"

4050 EXPECT_ERR=3
4060 EXPECT_ERL=4070
4065 TRIPPED=0

4070 RETURN

4080 REM If we got here, handler resumed NEXT after the error.
4090 IF TRIPPED=0 THEN FAIL=FAIL+1: PRINT "FAIL: RETURN without GOSUB did not error" ELSE PASS=PASS+1

5000 PRINT
5010 PRINT "=================================="
5020 PRINT "GOSUB / RETURN TORTURE TEST COMPLETE"
5030 PRINT "PASS=";PASS;" FAIL=";FAIL
5040 PRINT "=================================="
5050 END

8000 REM ==========================================================
8010 REM ERROR HANDLER
8020 REM ==========================================================
8030 IF ERR=EXPECT_ERR AND ERL=EXPECT_ERL THEN TRIPPED=1 ELSE FAIL=FAIL+1: PRINT "FAIL: RETURN error ERR=";ERR;" ERL=";ERL
8040 ERR=0
8050 RESUME NEXT
