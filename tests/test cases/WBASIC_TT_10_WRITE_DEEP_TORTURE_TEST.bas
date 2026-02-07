10 REM ============================================================
20 REM WRITE / WRITE # DEEP TORTURE TEST (GW-BASIC strict) - v2
30 REM Fixes:
40 REM   - Float comparisons use tolerance (GW-BASIC safe)
50 REM   - PRINT # comma is ZONE spacing, NOT a literal comma
60 REM No block IF, unique line numbers
70 REM ============================================================
80 DEFINT A-Z
90 PASS=0:FAIL=0
100 PRINT "Running WRITE / WRITE # deep torture test..."
110 PRINT
120 ON ERROR GOTO 9000

130 EPS!=.0001

140 OPEN "write_deep.tmp" FOR OUTPUT AS #1
150 OPEN "write_deep.tmp" FOR INPUT AS #2

160 REM ------------------ helper: near compare -------------------
170 REM Use: GOT!=x : EXP!=y : DESC$=... : GOSUB 8000
180 REM -----------------------------------------------------------

200 PRINT "TEST1: Numeric formatting + round-trip"
210 WRITE #1, 1, -2, 3.5, 4E2
220 INPUT #2, A, B, C!, D!
230 IF A=1 THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: A"
240 IF B=-2 THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: B"
250 GOT!=C!: EXP!=3.5: DESC$="C ~= 3.5": GOSUB 8000
260 GOT!=D!: EXP!=400: DESC$="D ~= 400": GOSUB 8000

300 PRINT "TEST2: String quoting and embedded commas"
310 WRITE #1, "A,B", "C"
320 INPUT #2, S1$, S2$
330 IF S1$="A,B" THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: S1$"
340 IF S2$="C" THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: S2$"

400 PRINT "TEST3: Embedded quotes doubling"
410 WRITE #1, "HE SAID ""WOW"""
420 INPUT #2, S$
430 IF S$="HE SAID ""WOW""" THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: quote unescape"

500 PRINT "TEST4: Empty fields"
510 WRITE #1, , , 5, ""
520 INPUT #2, A, B, C!, S$
530 IF A=0 THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: blank numeric A"
540 IF B=0 THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: blank numeric B"
550 GOT!=C!: EXP!=5: DESC$="C ~= 5": GOSUB 8000
560 IF S$="" THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: empty string"

600 PRINT "TEST5: Trailing separators"
610 WRITE #1, 1,2,
620 INPUT #2, A, B
630 IF A=1 THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: trailing A"
640 IF B=2 THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: trailing B"

700 PRINT "TEST6: Mixed numeric + string"
710 WRITE #1, "X", 10, "Y", -3.25
720 INPUT #2, S1$, A, S2$, B!
730 IF S1$="X" THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: mix S1$"
740 IF A=10 THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: mix A"
750 IF S2$="Y" THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: mix S2$"
760 GOT!=B!: EXP!=-3.25: DESC$="B ~= -3.25": GOSUB 8000

800 PRINT "TEST7: PRINT # uses zones (no literal commas)"
810 PRINT #1, "ABC",1
820 LINE INPUT #2, L$
830 IF INSTR(L$, ",")=0 THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: PRINT # should not contain literal comma"
840 IF LEFT$(L$,3)="ABC" THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: PRINT # line should start with ABC"
850 IF INSTR(L$, "1")>0 THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: PRINT # line should contain 1"

900 PRINT "TEST8: WRITE forces newline"
910 WRITE #1, 1
920 WRITE #1, 2
930 INPUT #2, A
940 INPUT #2, B
950 IF A=1 THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: newline A"
960 IF B=2 THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: newline B"

1000 CLOSE #1
1010 CLOSE #2

1100 PRINT
1110 PRINT "=================================="
1120 PRINT "WRITE / WRITE # DEEP TORTURE TEST COMPLETE"
1130 PRINT "PASS=";PASS;" FAIL=";FAIL
1140 PRINT "=================================="
1150 END

8000 REM ------------------ near compare sub -----------------------
8010 DIF!=GOT!-EXP!
8020 IF DIF!<0 THEN DIF!=-DIF!
8030 IF DIF!<=EPS! THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL:";DESC$;" EXPECT=";EXP!;" GOT=";GOT!
8040 RETURN

9000 PRINT "UNEXPECTED ERROR: ERR=";ERR;" ERL=";ERL
9010 END
