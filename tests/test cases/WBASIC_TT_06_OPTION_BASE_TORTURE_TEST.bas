10 REM ==========================================
20 REM OPTION BASE TORTURE TEST (GW-BASIC style)
30 REM Must run BEFORE any DIM or array reference
40 REM ==========================================

100 DEFINT A-Z
110 PASS=0: FAIL=0

120 PRINT "Running OPTION BASE torture test..."
130 PRINT

140 OPTION BASE 1

150 DIM A(3)
160 A(1)=42
170 IF A(1)=42 THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: A(1) expected 42 got ";A(1)

180 ON ERROR GOTO 500
190 A(0)=1
200 FAIL=FAIL+1: PRINT "FAIL: A(0) should be out of range"
210 GOTO 600

500 IF ERR=9 THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: Expected ERR=9, got ERR=";ERR
510 RESUME 600

600 PRINT
610 PRINT "=================================="
620 PRINT "OPTION BASE TORTURE TEST COMPLETE"
630 PRINT "PASS=";PASS;" FAIL=";FAIL
640 PRINT "=================================="
650 END
