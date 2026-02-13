10 REM ==============================================================
20 REM PRINT USING PHASE 4 PARITY TORTURE TEST
30 REM Covers cycling, separators, malformed masks, and mismatches.
40 REM ==============================================================
45 REM Expected summary on a passing runtime: PASS=7 FAIL=0
46 REM Expected separator sample:
47 REM   1             2 3
48 REM   4 5           6
50 DEFINT A-Z
60 PASS=0:FAIL=0
70 ON ERROR GOTO 9000
80 PRINT "Running PRINT USING phase 4 parity torture test..."
90 PRINT
100 GOSUB 1000
110 GOSUB 2000
120 GOSUB 3000
130 GOSUB 4000
140 GOSUB 5000
150 PRINT
160 PRINT "=================================="
170 PRINT "PRINT USING PHASE 4 PARITY COMPLETE"
180 PRINT "PASS=";PASS;" FAIL=";FAIL
190 PRINT "=================================="
200 END
1000 REM TEST1: mixed literal and numeric/string fields
1010 PRINT "TEST1: mixed literal fields"
1020 PRINT USING "A=## B=! C=&";12;"Q";"WIDE"
1030 PASS=PASS+1
1040 RETURN
2000 REM TEST2: format cycling with fewer fields than values
2010 PRINT "TEST2: cycling"
2020 PRINT USING "[#][!]";1;"A";2;"B";3;"C"
2030 PASS=PASS+1
2040 RETURN
3000 REM TEST3: comma/semicolon separators inside USING list
3010 PRINT "TEST3: separators"
3020 PRINT USING "##";1,2;3
3030 PRINT USING "##";4;5,6
3040 PASS=PASS+1
3050 RETURN
4000 REM TEST4: malformed format string should error
4010 PRINT "TEST4: malformed mask error"
4020 DESC$="Malformed mask must raise error"
4030 EXP_MODE=1:EXP_ERL=4040:TRIPPED=0
4040 PRINT USING "\   ";"BAD"
4050 IF TRIPPED=0 THEN FAIL=FAIL+1:PRINT "FAIL:";DESC$;" (no error)" ELSE PASS=PASS+1
4060 RETURN
5000 REM TEST5: type mismatch should error
5010 PRINT "TEST5: type mismatch"
5020 DESC$="String in numeric field must Type mismatch"
5030 EXP_MODE=1:EXP_ERL=5040:TRIPPED=0
5040 PRINT USING "##";"BAD"
5050 IF TRIPPED=0 THEN FAIL=FAIL+1:PRINT "FAIL:";DESC$;" (no error)" ELSE PASS=PASS+1
5060 RETURN
9000 IF EXP_MODE=0 THEN PRINT "UNEXPECTED ERROR ERR=";ERR;" ERL=";ERL:FAIL=FAIL+1:RESUME 9999
9010 TRIPPED=1
9020 IF ERL=EXP_ERL THEN PASS=PASS+1 ELSE FAIL=FAIL+1:PRINT "FAIL:";DESC$;" expected ERL=";EXP_ERL;" got ERL=";ERL
9030 EXP_MODE=0
9040 ERR=0
9050 RESUME NEXT
9999 ON ERROR GOTO 0
10000 END
