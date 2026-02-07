10 REM ============================================================
20 REM DATA PLACEMENT + CONTROL FLOW (DEEP) TORTURE TEST - v2
30 REM GW-BASIC strict. Unique line numbers. Deterministic via RESTORE <line>.
40 REM ============================================================
50 DEFINT A-Z
60 PASS=0:FAIL=0
70 PRINT "Running DATA placement + flow (DEEP) torture test..."
80 PRINT
90 ON ERROR GOTO 9000

100 REM ------------------------------------------------------------
110 PRINT "TEST1: DATA after END is readable"
120 RESTORE 5000
130 READ A
140 IF A=42 THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: TEST1 EXPECT=42 GOT=";A

200 REM ------------------------------------------------------------
210 PRINT "TEST2: DATA on an unreachable line is still in the stream"
220 RESTORE 5100
230 READ A
240 IF A=55 THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: TEST2 EXPECT=55 GOT=";A

300 REM ------------------------------------------------------------
310 PRINT "TEST3: DATA after RETURN is still in the stream"
320 RESTORE 5200
330 READ A
340 IF A=8 THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: TEST3 EXPECT=8 GOT=";A

400 REM ------------------------------------------------------------
410 PRINT "TEST4: DATA in a subroutine exists in the stream"
420 RESTORE 5300
430 READ A
440 IF A=5 THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: TEST4 EXPECT=5 GOT=";A

500 REM ------------------------------------------------------------
510 PRINT "TEST5: DATA lines inside a FOR body exist once (not per iteration)"
520 REM We prove this by reading the FIRST item at the anchor line.
530 RESTORE 5400
540 READ A
550 IF A=9 THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: TEST5 EXPECT=9 GOT=";A

600 REM ------------------------------------------------------------
610 PRINT "TEST6: GOTO skipping DATA does not change DATA order"
620 RESTORE 5500
630 READ A: READ B
640 IF A=3 AND B=4 THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: TEST6 EXPECT 3,4 GOT ";A;",";B

700 REM ------------------------------------------------------------
710 PRINT "TEST7: RESTORE inside loop replays DATA (sum should be 1+1+1 = 3)"
720 RESTORE 5600
730 SUM=0
740 FOR I=1 TO 3
750 READ A
760 SUM=SUM+A
770 RESTORE 5600
780 NEXT I
790 IF SUM=3 THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: TEST7 EXPECT SUM=3 GOT=";SUM

800 REM ------------------------------------------------------------
810 PRINT "TEST8: Out of DATA error from a short anchor (ERR=4)"
820 RESTORE 5700
830 ON ERROR GOTO 880
840 READ A
850 READ B
860 FAIL=FAIL+1: PRINT "FAIL: TEST8 missing Out of DATA error"
870 GOTO 900
880 IF ERR=4 THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: TEST8 expected ERR=4 got ERR=";ERR
890 RESUME 900
900 ON ERROR GOTO 9000

950 PRINT
960 PRINT "=============================================="
970 PRINT "DATA PLACEMENT + FLOW (DEEP) TORTURE COMPLETE"
980 PRINT "PASS=";PASS;" FAIL=";FAIL
990 PRINT "=============================================="
1000 END

5000 REM ---- Anchor for TEST1 (DATA after END) --------------------
5010 END
5020 DATA 42

5100 REM ---- Anchor for TEST2 (unreachable DATA line) -------------
5110 GOTO 5130
5120 DATA 55
5130 REM (fallthrough)

5200 REM ---- Anchor for TEST3 (DATA after RETURN) -----------------
5210 GOSUB 5230
5220 REM after RETURN, the DATA line is still part of program DATA
5225 DATA 8
5227 GOTO 5240
5230 RETURN
5240 REM

5300 REM ---- Anchor for TEST4 (DATA in subroutine area) -----------
5310 GOSUB 5330
5320 GOTO 5340
5330 DATA 5
5335 RETURN
5340 REM

5400 REM ---- Anchor for TEST5 (DATA located in FOR body region) ---
5410 FOR I=1 TO 3
5420 DATA 9
5430 NEXT I

5500 REM ---- Anchor for TEST6 (GOTO skips over DATA lines) --------
5510 GOTO 5530
5520 DATA 3
5525 DATA 4
5530 REM
5535 REM But DATA stream must still be 3 then 4
5540 GOTO 5540  ' do nothing loop (never executed in this test)

5600 REM ---- Anchor for TEST7 (loop RESTORE replay) ---------------
5610 DATA 1

5700 REM ---- Anchor for TEST8 (Out of DATA: exactly one item) -----
5710 DATA 99

9000 PRINT "UNEXPECTED ERROR: ERR=";ERR;" ERL=";ERL
9010 END
