10 REM ==========================================================
20 REM ERRORS TORTURE TEST (GW-BASIC style)
30 REM Tests:
40 REM  - ON ERROR reachability
50 REM  - ERR/ERL correctness
60 REM  - RESUME NEXT
70 REM  - Error inside GOSUB doesn't break RETURN stack
80 REM  - RESUME <line#>
90 REM ==========================================================

100 DEFINT A-Z
110 PASS=0: FAIL=0
120 EXP_MODE=0
130 TRIPPED=0
140 ON ERROR GOTO 8000

150 PRINT "Running ERRORS torture test..."
160 PRINT

170 GOSUB 10000
180 GOSUB 11000
190 GOSUB 12000
200 GOSUB 14000

210 PRINT
220 PRINT "=================================="
230 PRINT "ERRORS TORTURE TEST COMPLETE"
240 PRINT "PASS=";PASS;" FAIL=";FAIL
250 PRINT "=================================="
260 END

900 REM ==========================================================
910 REM ASSERT NUMERIC (safe outside handler)
920 REM ==========================================================
930 IF GOT=EXPECT THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL:";DESC$;" EXPECT=";EXPECT;" GOT=";GOT
940 RETURN

9000 REM ==========================================================
9010 REM ARM EXPECTED ERROR
9020 REM EXPECTMODE: 1 => RESUME NEXT
9030 REM             2 => RESUME 14200 (fixed recovery line)
9040 REM ==========================================================
9050 EXP_MODE=EXPECTMODE
9060 EXP_ERR=EXPECTERR
9070 EXP_ERL=EXPECTERL
9080 TRIPPED=0
9090 RETURN

10000 REM ==========================================================
10010 REM TEST1: DIV/0 + RESUME NEXT
10020 REM ==========================================================
10030 PRINT "TEST1: DIV/0 + RESUME NEXT"

10040 EXPECTMODE=1: EXPECTERR=11: EXPECTERL=10100
10050 DESC$="DIV/0 sets ERR=11 and ERL=10100"
10060 GOSUB 9000

10070 A=0
10080 REM Next line should error
10100 X=1/A

10110 IF TRIPPED THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: DIV/0 trap did not fire"
10120 RETURN

11000 REM ==========================================================
11010 REM TEST2: TYPE MISMATCH + RESUME NEXT
11020 REM ==========================================================
11030 PRINT "TEST2: Type mismatch + RESUME NEXT"

11040 EXPECTMODE=1: EXPECTERR=13: EXPECTERL=11100
11050 DESC$="Type mismatch sets ERR=13 and ERL=11100"
11060 GOSUB 9000

11070 A$="ABC"
11080 REM Next line should error
11100 N=A$

11110 IF TRIPPED THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: Type mismatch trap did not fire"
11120 RETURN

12000 REM ==========================================================
12010 REM TEST3: ERROR INSIDE GOSUB PRESERVES RETURN STACK
12020 REM ==========================================================
12030 PRINT "TEST3: Error inside GOSUB preserves RETURN stack"

12040 GOSUB 13000
12050 GOT=GOSUB_OK: EXPECT=1: DESC$="Returned from GOSUB after trapped error": GOSUB 930
12060 RETURN

13000 REM --- Subroutine that will error and then RETURN cleanly ---
13010 GOSUB_OK=0

13020 EXPECTMODE=1: EXPECTERR=9: EXPECTERL=13100
13030 DESC$="Subscript OOR inside GOSUB (ERR=9 ERL=13100)"
13040 GOSUB 9000

13050 DIM T(1)
13060 T(0)=7
13080 REM Next line should error
13100 T(2)=1

13110 IF TRIPPED THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: Subscript trap did not fire in GOSUB"
13120 GOSUB_OK=1
13130 RETURN

14000 REM ==========================================================
14010 REM TEST4: RESUME <line#> RECOVERY
14020 REM ==========================================================
14030 PRINT "TEST4: RESUME <line#> recovery"

14040 RECOVERED=0
14050 EXPECTMODE=2: EXPECTERR=11: EXPECTERL=14100
14060 DESC$="Handler RESUME to 14200 after DIV/0 at 14100"
14070 GOSUB 9000

14080 A=0
14100 Q=99/A
14110 FAIL=FAIL+1: PRINT "FAIL: DIV/0 did not trap (should not reach 14110)"
14120 RETURN

14200 RECOVERED=1
14210 GOT=RECOVERED: EXPECT=1: DESC$="RESUME <line#> landed in recovery block": GOSUB 930
14220 IF TRIPPED THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: Recovery trap did not fire"
14230 RETURN

8000 REM ==========================================================
8010 REM ERROR HANDLER (stack-safe, no GOSUB)
8020 REM ==========================================================
8030 IF EXP_MODE=0 THEN PRINT "UNEXPECTED ERROR: ERR=";ERR;" ERL=";ERL: FAIL=FAIL+1: GOTO 19990

8040 TRIPPED=1

8050 GOT=ERR: EXPECT=EXP_ERR
8060 IF GOT=EXPECT THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL:";DESC$;" expected ERR=";EXPECT;" got ERR=";GOT

8070 GOT=ERL: EXPECT=EXP_ERL
8080 IF GOT=EXPECT THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL:";DESC$;" expected ERL=";EXPECT;" got ERL=";GOT

8090 ERR=0

8100 IF EXP_MODE=1 THEN EXP_MODE=0: RESUME NEXT
8110 IF EXP_MODE=2 THEN EXP_MODE=0: RESUME 14200

8120 EXP_MODE=0
8130 RESUME NEXT

19990 ON ERROR GOTO 0
20000 END
