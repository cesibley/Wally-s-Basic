10 REM ============================================================
20 REM END / STOP / SYSTEM TORTURE TEST (GW-BASIC strict) - v2
30 REM Each of END / STOP / SYSTEM terminates or halts execution,
40 REM so you MUST run them one-at-a-time using MODE below.
50 REM SYSTEM is expected to behave same as END in WBASIC.
60 REM ============================================================
70 DEFINT A-Z
80 MODE=3  ' set to 1=END, 2=STOP, 3=SYSTEM then RUN
90 PRINT "Running END / STOP / SYSTEM torture test..."
100 PRINT "MODE=";MODE
110 PRINT

120 IF MODE<1 OR MODE>3 THEN PRINT "Set MODE=1..3 and RUN": END

200 IF MODE=1 THEN GOTO 1000
210 IF MODE=2 THEN GOTO 2000
220 IF MODE=3 THEN GOTO 3000

900 END

1000 REM ---------------- TEST1: END terminates immediately --------
1010 PRINT "TEST1: END terminates immediately"
1020 PRINT "If you see this line, END has not executed yet."
1030 END
1040 PRINT "FAIL: Reached after END (should be impossible)"
1050 END

2000 REM ---------------- TEST2: STOP halts ------------------------
2010 PRINT "TEST2: STOP halts and returns to OK prompt"
2020 PRINT "You should see back to the IDLE/IMMEDIATE state. Program must not continue."
2030 STOP
2040 PRINT "FAIL: Continued after STOP (should be impossible)"
2050 END

3000 REM ---------------- TEST3: SYSTEM behaves like END ------------
3010 PRINT "TEST3: SYSTEM behaves like END"
3020 PRINT "If SYSTEM is implemented as END, program will terminate now."
3030 SYSTEM
3040 PRINT "FAIL: Reached after SYSTEM (should be impossible)"
3050 END
