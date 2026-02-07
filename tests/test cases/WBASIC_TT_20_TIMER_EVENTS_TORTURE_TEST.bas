10 REM ==========================================================
20 REM ON TIMER / TIMER EVENTS TORTURE TEST (GW-BASIC strict)
30 REM No block IF. Unique line numbers.
40 REM Tests:
50 REM   1) Basic ON TIMER fires (single-shot via TIMER OFF in handler)
60 REM   2) TIMER OFF prevents firing
70 REM   3) TIMER STOP pauses events, TIMER ON resumes
80 REM   4) No re-entrancy (handler should not re-enter while running)
90 REM ==========================================================

100 DEFINT A-Z
110 PASS=0: FAIL=0
120 PRINT "Running ON TIMER / TIMER events torture test..."
130 PRINT

140 ON ERROR GOTO 9000

200 GOSUB 1000
210 GOSUB 2000
220 GOSUB 3000
230 GOSUB 4000

240 PRINT
250 PRINT "=================================="
260 PRINT "ON TIMER / TIMER EVENTS TORTURE TEST COMPLETE"
270 PRINT "PASS=";PASS;" FAIL=";FAIL
280 PRINT "=================================="
290 END

900 REM ===================== ASSERT NUMERIC ======================
910 IF GOT=EXPECT THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL:";DESC$;" EXPECT=";EXPECT;" GOT=";GOT
920 RETURN

930 REM ===================== ASSERT MIN (>=) =====================
940 IF GOT>=EXPECT THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL:";DESC$;" EXPECT>=";EXPECT;" GOT=";GOT
950 RETURN

960 REM ===================== RESET EVENT STATE ===================
970 HIT=0: REENT=0: INHAND=0: MAIN=0
980 RETURN

990 REM ===================== WAIT SECS (busy wait) ===============
1000 REM (declared later as a GOSUB target)
1010 REM placeholder
1020 RETURN

1000 REM ==========================================================
1010 REM TEST1: Basic ON TIMER fires (single-shot)
1020 REM ==========================================================
1030 PRINT "TEST1: Basic ON TIMER fires (single-shot)"
1040 GOSUB 960
1050 ON TIMER(1) GOSUB 8000
1060 TIMER ON
1070 START!=TIMER
1080 WHILE (TIMER-START!) < 2.5
1090   MAIN=MAIN+1
1100 WEND
1110 TIMER OFF
1120 ON TIMER(1) GOSUB 0

1130 GOT=HIT: EXPECT=1: DESC$="Timer handler fired at least once": GOSUB 930
1140 GOT=MAIN: EXPECT=1: DESC$="Main loop continued running": GOSUB 930
1150 RETURN

2000 REM ==========================================================
2010 REM TEST2: TIMER OFF prevents firing
2020 REM ==========================================================
2030 PRINT "TEST2: TIMER OFF prevents firing"
2040 GOSUB 960
2050 ON TIMER(1) GOSUB 8000
2060 TIMER OFF
2070 START!=TIMER
2080 WHILE (TIMER-START!) < 1.8
2090   MAIN=MAIN+1
2100 WEND
2110 ON TIMER(1) GOSUB 0

2120 GOT=HIT: EXPECT=0: DESC$="No handler fire while TIMER OFF": GOSUB 910
2130 RETURN

3000 REM ==========================================================
3010 REM TEST3: TIMER STOP pauses events; TIMER ON resumes
3020 REM NOTE: We keep interval at 1 second for stability.
3030 REM ==========================================================
3040 PRINT "TEST3: TIMER STOP pauses; TIMER ON resumes"
3050 GOSUB 960
3060 ON TIMER(1) GOSUB 8000

3070 REM Pause event processing
3080 TIMER STOP
3090 START!=TIMER
3100 WHILE (TIMER-START!) < 1.6
3110   MAIN=MAIN+1
3120 WEND
3130 GOT=HIT: EXPECT=0: DESC$="No fire while TIMER STOP": GOSUB 910

3140 REM Resume event processing
3150 TIMER ON
3160 START!=TIMER
3170 WHILE (TIMER-START!) < 2.2
3180   MAIN=MAIN+1
3190 WEND
3200 TIMER OFF
3210 ON TIMER(1) GOSUB 0

3220 GOT=HIT: EXPECT=1: DESC$="Handler fires after resume": GOSUB 930
3230 RETURN

4000 REM ==========================================================
4010 REM TEST4: No re-entrancy while handler running
4020 REM We use a small-ish interval and make the handler "slow".
4030 REM If events re-enter the handler, REENT will become 1.
4040 REM ==========================================================
4050 PRINT "TEST4: No handler re-entrancy"
4060 GOSUB 960

4070 ON TIMER(.2) GOSUB 8100
4080 TIMER ON
4090 START!=TIMER
4100 WHILE (TIMER-START!) < 1.5
4110   MAIN=MAIN+1
4120 WEND
4130 TIMER OFF
4140 ON TIMER(.2) GOSUB 0

4150 GOT=REENT: EXPECT=0: DESC$="Handler did not re-enter itself": GOSUB 910
4160 GOT=HIT: EXPECT=1: DESC$="Handler fired at least once": GOSUB 930
4170 RETURN

7000 REM ==========================================================
7010 REM WAIT SUBROUTINE (busy wait)
7020 REM Usage: SECS!=<seconds> then GOSUB 7000
7030 REM ==========================================================
7040 T0!=TIMER
7050 WHILE (TIMER-T0!) < SECS!
7060 WEND
7070 RETURN

8000 REM ==========================================================
8010 REM TIMER HANDLER (single-shot)
8020 REM ==========================================================
8030 IF INHAND<>0 THEN REENT=1
8040 INHAND=1
8050 HIT=HIT+1
8060 TIMER OFF
8070 INHAND=0
8080 RETURN

8100 REM ==========================================================
8110 REM TIMER HANDLER (slow handler to detect re-entrancy)
8120 REM ==========================================================
8130 IF INHAND<>0 THEN REENT=1
8140 INHAND=1
8150 HIT=HIT+1
8160 REM Slow work ~0.35 seconds
8170 SECS!=.35: GOSUB 7000
8180 INHAND=0
8190 RETURN

9000 REM ==========================================================
9010 REM GLOBAL ERROR HANDLER
9020 REM If ON TIMER / TIMER ON/OFF/STOP is missing, this will trip.
9030 REM ==========================================================
9040 PRINT "UNEXPECTED ERROR (likely missing TIMER events support): ERR=";ERR;" ERL=";ERL
9050 END
