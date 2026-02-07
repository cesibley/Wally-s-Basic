10 REM ==========================================================
20 REM WHILE / WEND (DEEP) TORTURE TEST - STRICT GW-BASIC (v2)
30 REM No block IF. No reused line numbers.
40 REM Tests:
50 REM   1) Basic WHILE/WEND counting
60 REM   2) Condition re-evaluation (depends on var mutated in body)
70 REM   3) Nested WHILE/WEND + correct matching
80 REM   4) WHILE with IF/THEN/ELSE single-line
90 REM   5) ':' statement chaining inside WHILE body
100 REM   6) WEND without WHILE should error (trap)
110 REM ==========================================================

120 DEFINT A-Z
130 PASS=0: FAIL=0
140 DESC$=""
150 TRIPPED=0: LASTERR=0: LASTERL=0
160 ON ERROR GOTO 9000

170 PRINT "Running WHILE/WEND deep torture test..."
180 PRINT

190 GOSUB 1000
200 GOSUB 2000
210 GOSUB 3000
220 GOSUB 4000
230 GOSUB 5000
240 GOSUB 6000

250 PRINT
260 PRINT "=================================="
270 PRINT "WHILE/WEND DEEP TORTURE TEST COMPLETE"
280 PRINT "PASS=";PASS;" FAIL=";FAIL
290 PRINT "=================================="
300 END

900 REM ===================== ASSERT NUMERIC ======================
910 IF GOT=EXPECT THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL:";DESC$;" EXPECT=";EXPECT;" GOT=";GOT
920 RETURN

1000 REM ==========================================================
1010 REM TEST1: Basic WHILE/WEND counting
1020 REM ==========================================================
1030 PRINT "TEST1: Basic WHILE/WEND counting"

1040 I=0: SUM=0
1050 WHILE I<5
1060   SUM=SUM+I
1070   I=I+1
1080 WEND
1090 GOT=I: EXPECT=5: DESC$="I ends at 5": GOSUB 910
1100 GOT=SUM: EXPECT=10: DESC$="SUM=0+1+2+3+4=10": GOSUB 910
1110 RETURN

2000 REM ==========================================================
2010 REM TEST2: Condition re-evaluation timing (GW-BASIC strict)
2020 REM Condition depends on X, which is only changed in body.
2030 REM If condition is not re-evaluated, this could loop forever.
2040 REM We include a safety cutoff.
2050 REM ==========================================================
2060 PRINT "TEST2: Condition re-evaluation timing"

2070 I=0: X=0
2080 WHILE X=0
2090   I=I+1
2100   IF I=3 THEN X=1 ELSE X=0
2110   IF I>20 THEN FAIL=FAIL+1: PRINT "FAIL: WHILE condition not re-evaluated (safety cutoff)"; : X=1
2120 WEND
2130 GOT=I: EXPECT=3: DESC$="Loop exits exactly when X becomes 1 at I=3": GOSUB 910
2140 RETURN

3000 REM ==========================================================
3010 REM TEST3: Nested WHILE/WEND + matching correctness
3020 REM ==========================================================
3030 PRINT "TEST3: Nested WHILE/WEND matching"

3040 OUT=0
3050 I=1
3060 WHILE I<=3
3070   J=1
3080   WHILE J<=4
3090     OUT=OUT+1
3100     J=J+1
3110   WEND
3120   I=I+1
3130 WEND
3140 GOT=OUT: EXPECT=12: DESC$="3*4 iterations = 12": GOSUB 910
3150 RETURN

4000 REM ==========================================================
4010 REM TEST4: IF/THEN/ELSE single-line inside WHILE
4020 REM ==========================================================
4030 PRINT "TEST4: IF/THEN/ELSE inside WHILE"

4040 I=0: HITS=0
4050 WHILE I<6
4060   IF I MOD 2=0 THEN HITS=HITS+10 ELSE HITS=HITS+1
4070   I=I+1
4080 WEND
4090 GOT=HITS: EXPECT=33: DESC$="Evens add 10 (3x)=30, odds add 1 (3x)=3 => 33": GOSUB 910
4100 RETURN

5000 REM ==========================================================
5010 REM TEST5: ':' statement chaining inside WHILE body
5020 REM ==========================================================
5030 PRINT "TEST5: ':' chaining inside WHILE body"

5040 I=0: X=0
5050 WHILE I<3
5060   X=X+1 : I=I+1 : X=X+1
5070 WEND
5080 GOT=X: EXPECT=6: DESC$="Chained statements execute in order": GOSUB 910
5090 RETURN

6000 REM ==========================================================
6010 REM TEST6: WEND without WHILE should error (trap)
6020 REM We assert: it MUST error, and ERL must be the WEND line.
6030 REM ==========================================================
6040 PRINT "TEST6: WEND without WHILE should error"

6050 TRIPPED=0: LASTERR=0: LASTERL=0
6060 ON ERROR GOTO 6100
6070 WEND
6080 FAIL=FAIL+1: PRINT "FAIL: WEND without WHILE did not error"
6090 GOTO 6130

6100 TRIPPED=1: LASTERR=ERR: LASTERL=ERL: ERR=0
6110 RESUME 6130

6130 ON ERROR GOTO 9000
6140 GOT=TRIPPED: EXPECT=1: DESC$="WEND without WHILE triggers error": GOSUB 910
6150 GOT=LASTERL: EXPECT=6070: DESC$="ERL points to stray WEND line": GOSUB 910
6160 RETURN

9000 REM ==========================================================
9010 REM GLOBAL ERROR HANDLER
9020 REM ==========================================================
9030 PRINT "UNEXPECTED ERROR: ERR=";ERR;" ERL=";ERL
9040 END
