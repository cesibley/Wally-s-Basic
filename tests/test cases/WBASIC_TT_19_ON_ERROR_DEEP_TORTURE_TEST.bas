10 REM ==========================================================
20 REM ON ERROR (DEEP) TORTURE TEST - STRICT GW-BASIC (FIXED)
30 REM Tests:
40 REM   1. Basic ON ERROR GOTO + ERR/ERL
50 REM   2. Error inside GOSUB preserves RETURN stack
60 REM   3. RESUME <line>
70 REM   4. Nested ON ERROR replacement (outer->inner) + correct ERL
80 REM   5. RESUME NEXT flow (skip faulting line)
90 REM ==========================================================

100 DEFINT A-Z
110 PASS=0: FAIL=0
120 DESC$=""
130 ON ERROR GOTO 9000

140 PRINT "Running ON ERROR deep torture test..."
150 PRINT

160 GOSUB 1000
170 GOSUB 2000
180 GOSUB 3000
190 GOSUB 4000
200 GOSUB 5000

210 PRINT
220 PRINT "=================================="
230 PRINT "ON ERROR DEEP TORTURE TEST COMPLETE"
240 PRINT "PASS=";PASS;" FAIL=";FAIL
250 PRINT "=================================="
260 END

900 REM ===================== ASSERT NUMERIC ======================
910 IF GOT=EXPECT THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL:";DESC$;" EXPECT=";EXPECT;" GOT=";GOT
920 RETURN

930 REM ===================== ASSERT ERL ==========================
940 IF GOT=EXPECT THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL:";DESC$;" EXPECT ERL=";EXPECT;" GOT=";GOT
950 RETURN

1000 REM ==========================================================
1010 REM TEST1: Basic ON ERROR GOTO
1020 REM ==========================================================
1030 PRINT "TEST1: Basic ON ERROR GOTO"

1040 ON ERROR GOTO 1100
1050 A=1/0
1060 FAIL=FAIL+1: PRINT "FAIL: DIV/0 did not trap"
1070 GOTO 1120

1100 GOT=ERR: EXPECT=11: DESC$="ERR set to 11 on DIV/0": GOSUB 910
1110 GOT=ERL: EXPECT=1050: DESC$="ERL points to DIV/0 line": GOSUB 930
1115 RESUME 1120

1120 ON ERROR GOTO 9000
1130 PASS=PASS+1
1140 RETURN

2000 REM ==========================================================
2010 REM TEST2: Error inside GOSUB preserves RETURN stack
2020 REM ==========================================================
2030 PRINT "TEST2: Error inside GOSUB preserves RETURN"

2040 ON ERROR GOTO 2100
2050 GOSUB 2200
2060 GOT=A: EXPECT=123: DESC$="Returned from GOSUB after error": GOSUB 910
2070 ON ERROR GOTO 9000
2080 PASS=PASS+1
2090 RETURN

2100 REM Handler: should resume at next line in the sub (sets A=123 then RETURN)
2110 RESUME NEXT

2200 REM --- GOSUB body ---
2210 A=1/0
2220 A=123
2230 RETURN

3000 REM ==========================================================
3010 REM TEST3: RESUME <line>
3020 REM ==========================================================
3030 PRINT "TEST3: RESUME <line>"

3040 ON ERROR GOTO 3100
3050 A=1/0
3060 FAIL=FAIL+1: PRINT "FAIL: RESUME <line> did not redirect"
3070 GOTO 3130

3100 GOT=ERR: EXPECT=11: DESC$="ERR=11 before RESUME <line>": GOSUB 910
3110 RESUME 3120

3120 A=42
3130 GOT=A: EXPECT=42: DESC$="RESUME <line> continues execution": GOSUB 910
3140 ON ERROR GOTO 9000
3150 PASS=PASS+1
3160 RETURN

4000 REM ==========================================================
4010 REM TEST4: Nested ON ERROR replacement (outer sets inner)
4020 REM ==========================================================
4030 PRINT "TEST4: Nested ON ERROR replacement"

4040 OUTER_HIT=0: INNER_HIT=0
4050 ON ERROR GOTO 4100
4060 A=1/0
4070 FAIL=FAIL+1: PRINT "FAIL: Outer handler not triggered"
4080 GOTO 4170

4100 OUTER_HIT=1
4110 ON ERROR GOTO 4200
4120 A=1/0
4130 FAIL=FAIL+1: PRINT "FAIL: Inner handler not triggered"
4140 GOTO 4170

4200 INNER_HIT=1
4210 GOT=ERR: EXPECT=11: DESC$="Inner handler ERR correct": GOSUB 910
4220 GOT=ERL: EXPECT=4120: DESC$="Inner handler ERL points to inner DIV/0": GOSUB 930
4230 RESUME 4170

4170 GOT=OUTER_HIT: EXPECT=1: DESC$="Outer handler was reached": GOSUB 910
4180 GOT=INNER_HIT: EXPECT=1: DESC$="Inner handler was reached": GOSUB 910
4190 ON ERROR GOTO 9000
4200 PASS=PASS+1
4210 RETURN

5000 REM ==========================================================
5010 REM TEST5: RESUME NEXT flow (skip faulting line)
5020 REM ==========================================================
5030 PRINT "TEST5: RESUME NEXT flow"

5040 A=0
5050 ON ERROR GOTO 5200
5060 A=1/0
5070 A=99
5080 GOT=A: EXPECT=99: DESC$="RESUME NEXT skipped faulting line": GOSUB 910
5090 ON ERROR GOTO 9000
5100 PASS=PASS+1
5110 RETURN

5200 REM Handler for TEST5
5210 RESUME NEXT

9000 REM ==========================================================
9010 REM FALLBACK HANDLER (should not fire during tests)
9020 REM ==========================================================
9030 PRINT "UNEXPECTED ERROR: ERR=";ERR;" ERL=";ERL
9040 END
