10 REM ============================================================
20 REM PARSER REGRESSION (DEEP) TORTURE TEST
30 REM Focus: tokenization, ':' splitting, REM/comments, quotes
40 REM GW-BASIC strict. Block IF allowed.
50 REM ============================================================
60 DEFINT A-Z
70 PASS=0:FAIL=0
80 PRINT "Running PARSER regression (DEEP) torture test..."
90 PRINT

100 ON ERROR GOTO 9000

110 REM ------------------------------------------------------------
120 PRINT "TEST1: ':' inside quotes does not split statements"
130 A$="HELLO:WORLD"
140 IF A$="HELLO:WORLD" THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: TEST1"

200 REM ------------------------------------------------------------
210 PRINT "TEST2: ':' splits statements outside quotes"
220 A=1:B=2:C=3
230 IF A=1 AND B=2 AND C=3 THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: TEST2"

300 REM ------------------------------------------------------------
310 PRINT "TEST3: REM ignores ':' after it"
320 A=1 REM THIS:SHOULD:NOT:SPLIT
330 B=2
340 IF A=1 AND B=2 THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: TEST3"

400 REM ------------------------------------------------------------
410 PRINT "TEST4: Apostrophe comment ignores ':'"
420 A=1 ' COMMENT:WITH:COLONS
430 B=2
440 IF A=1 AND B=2 THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: TEST4"

500 REM ------------------------------------------------------------
510 PRINT "TEST5: Keywords inside strings are not parsed"
520 A$="IF THEN ELSE FOR NEXT DATA REM"
530 IF INSTR(A$,"DATA")>0 THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: TEST5"

600 REM ------------------------------------------------------------
610 PRINT "TEST6: THEN with ':' chaining"
620 A=0
630 IF 1 THEN A=5:B=6
640 IF A=5 AND B=6 THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: TEST6"

700 REM ------------------------------------------------------------
710 PRINT "TEST7: ELSE with ':' chaining"
720 A=0:B=0
730 IF 0 THEN A=1 ELSE A=2:B=3
740 IF A=2 AND B=3 THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: TEST7"

800 REM ------------------------------------------------------------
810 PRINT "TEST8: Nested parentheses and operators"
820 A=(1+(2*(3+4)))
830 IF A=15 THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: TEST8"

900 REM ------------------------------------------------------------
910 PRINT "TEST9: DATA fields with ':' and ',' inside quotes"
920 RESTORE 2000
930 READ A$,B$
940 IF A$="X:Y:Z" AND B$="A,B,C" THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: TEST9"

1000 REM ------------------------------------------------------------
1010 PRINT "TEST10: DATA + ':' on same line"
1020 RESTORE 2010
1030 READ A,B
1040 IF A=1 AND B=2 THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: TEST10"

1100 REM ------------------------------------------------------------
1110 PRINT "TEST11: Multiple ':' with PRINT"
1120 A=0
1130 PRINT "ONE":A=1:PRINT "TWO":A=2
1140 IF A=2 THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: TEST11"

1200 REM ------------------------------------------------------------
1210 PRINT "TEST12: THEN newline ELSE block parsing"
1220 A=0
1230 IF 0 THEN
1240   A=1
1250 ELSE
1260   A=2
1270 END IF
1280 IF A=2 THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: TEST12"

1300 REM ------------------------------------------------------------
1310 PRINT "TEST13: REM keyword inside string literal"
1320 A$="THIS IS NOT A REM STATEMENT"
1330 IF LEFT$(A$,4)="THIS" THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: TEST13"

1400 REM ------------------------------------------------------------
1410 PRINT "TEST14: Trailing ':' should not create empty statement"
1420 A=1:
1430 IF A=1 THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: TEST14"

1500 REM ------------------------------------------------------------
1510 PRINT "TEST15: Colon before REM"
1520 A=1:REM COMMENT
1530 B=2
1540 IF A=1 AND B=2 THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: TEST15"

1600 REM ------------------------------------------------------------
1610 PRINT "TEST16: THEN keyword inside string"
1620 A$="THEN"
1630 IF A$="THEN" THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: TEST16"

1700 REM ------------------------------------------------------------
1710 PRINT "TEST17: DATA after quoted REM-like text"
1720 RESTORE 2020
1730 READ A$
1740 IF A$="REM THIS IS DATA" THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: TEST17"

1800 REM ------------------------------------------------------------
1810 PRINT "TEST18: Apostrophe inside string"
1820 A$="DON'T SPLIT"
1830 IF A$="DON'T SPLIT" THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: TEST18"

1900 REM ------------------------------------------------------------
1910 PRINT "TEST19: Multiple statements with spaces"
1920 A=0 : B=0 : C=0
1930 A=1 :B=2 : C=3
1940 IF A=1 AND B=2 AND C=3 THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: TEST19"

1950 REM ------------------------------------------------------------
1960 PRINT
1970 PRINT "=============================================="
1980 PRINT "PARSER REGRESSION (DEEP) TORTURE COMPLETE"
1990 PRINT "PASS=";PASS;" FAIL=";FAIL
2000 PRINT "=============================================="
2005 STOP

2000 DATA "X:Y:Z","A,B,C"
2010 DATA 1,2
2020 DATA "REM THIS IS DATA"

9000 PRINT "UNEXPECTED ERROR: ERR=";ERR;" ERL=";ERL
9010 END
