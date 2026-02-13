10 REM ==========================================================
20 REM PRINT USING PHASE 3 TORTURE TEST (STRING MASKS)
30 REM Exercises: !, \...\, &, mixed fields, mismatch
40 REM ==========================================================
50 DEFINT A-Z
60 PASS=0: FAIL=0
70 ON ERROR GOTO 8000
80 PRINT "Running PRINT USING PHASE3 torture test..."
90 PRINT
100 GOSUB 1000
110 GOSUB 2000
120 GOSUB 3000
130 GOSUB 4000
140 PRINT
150 PRINT "=================================="
160 PRINT "PRINT USING PHASE3 TEST COMPLETE"
170 PRINT "PASS=";PASS;" FAIL=";FAIL
180 PRINT "=================================="
190 END
1000 REM ! mask first character (empty=>space)
1010 PRINT "TEST1: !"
1020 A$="HELLO": B$="": C$="Z"
1030 PRINT USING "!|!|!";A$;B$;C$
1040 PASS=PASS+1
1050 RETURN
2000 REM \...\ fixed width (left align, right pad, trunc)
2010 PRINT "TEST2: fixed-width \...\ mask"
2020 A$="AB": B$="WXYZ": C$="LONG"
2030 PRINT USING "[\   \][\   \][\   \]";A$;B$;C$
2040 PASS=PASS+1
2050 RETURN
3000 REM & variable width + mixed numeric
3010 PRINT "TEST3: & mixed"
3020 N#=12.345
3030 PRINT USING "N=##.## S=& !";N#;"WORLD";"Q"
3040 PASS=PASS+1
3050 RETURN
4000 REM type mismatch should error when next field type is incompatible
4010 PRINT "TEST4: mismatch"
4020 DESC$="String value in numeric field should Type mismatch"
4030 EXP_MODE=1: EXP_ERL=4040: TRIPPED=0
4040 PRINT USING "##";"BAD"
4050 IF TRIPPED=0 THEN FAIL=FAIL+1: PRINT "FAIL:";DESC$;" (no error)" ELSE PASS=PASS+1
4060 RETURN
8000 IF EXP_MODE=0 THEN PRINT "UNEXPECTED ERROR ERR=";ERR;" ERL=";ERL: FAIL=FAIL+1: RESUME 9999
8010 TRIPPED=1
8020 IF ERL=EXP_ERL THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL:";DESC$;" expected ERL=";EXP_ERL;" got ERL=";ERL
8030 EXP_MODE=0
8040 ERR=0
8050 RESUME NEXT
9999 ON ERROR GOTO 0
10000 END
