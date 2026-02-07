10 REM ==========================================================
20 REM FOR/NEXT TORTURE TEST (GW-BASIC style)
30 REM Exercises:
40 REM  - Initial skip semantics (FOR I=1 TO 0)
50 REM  - STEP positive/negative, sign mismatch
60 REM  - Floating loops / drift resistance (basic)
70 REM  - Nested loops
80 REM  - NEXT varlists (NEXT J,I)
90 REM  - Loop variable after loop (final value behavior)
100 REM ==========================================================

110 DEFINT A-Z
120 PASS=0: FAIL=0
130 PRINT "Running FOR/NEXT torture test..."
140 PRINT

150 GOSUB 1000
160 GOSUB 2000
170 GOSUB 3000
180 GOSUB 4000
190 GOSUB 5000
200 GOSUB 6000

210 PRINT
220 PRINT "=================================="
230 PRINT "FOR/NEXT TORTURE TEST COMPLETE"
240 PRINT "PASS=";PASS;" FAIL=";FAIL
250 PRINT "=================================="
260 END

900 REM ==========================================================
910 REM ASSERT: EXPECT vs GOT
920 REM ==========================================================
930 IF GOT=EXPECT THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL:";DESC$;" EXPECT=";EXPECT;" GOT=";GOT
940 RETURN

1000 REM ==========================================================
1010 REM TEST 1: INITIAL SKIP (TO already past end, STEP positive)
1020 REM ==========================================================
1030 PRINT "TEST1: Initial skip (positive STEP)"
1040 CNT=0
1050 FOR I=1 TO 0
1060   CNT=CNT+1
1070 NEXT I
1080 GOT=CNT: EXPECT=0: DESC$="FOR I=1 TO 0 executes 0 times": GOSUB 900
1090 RETURN

2000 REM ==========================================================
2010 REM TEST 2: INITIAL SKIP (TO already past end, STEP negative)
2020 REM ==========================================================
2030 PRINT "TEST2: Initial skip (negative STEP)"
2040 CNT=0
2050 FOR I=0 TO 1 STEP -1
2060   CNT=CNT+1
2070 NEXT I
2080 GOT=CNT: EXPECT=0: DESC$="FOR I=0 TO 1 STEP -1 executes 0 times": GOSUB 900
2090 RETURN

3000 REM ==========================================================
3010 REM TEST 3: BASIC COUNTING (positive)
3020 REM ==========================================================
3030 PRINT "TEST3: Counting up"
3040 S=0
3050 FOR I=1 TO 5
3060   S=S+I
3070 NEXT I
3080 GOT=S: EXPECT=15: DESC$="Sum 1..5 = 15": GOSUB 900
3090 RETURN

4000 REM ==========================================================
4010 REM TEST 4: COUNTING DOWN (negative STEP)
4020 REM ==========================================================
4030 PRINT "TEST4: Counting down"
4040 S=0
4050 FOR I=5 TO 1 STEP -2
4060   S=S+I
4070 NEXT I
4080 GOT=S: EXPECT=9: DESC$="Sum 5,3,1 = 9": GOSUB 900
4090 RETURN

5000 REM ==========================================================
5010 REM TEST 5: STEP SIGN MISMATCH (should execute 0 times)
5020 REM ==========================================================
5030 PRINT "TEST5: STEP sign mismatch"
5040 CNT=0
5050 FOR I=1 TO 5 STEP -1
5060   CNT=CNT+1
5070 NEXT I
5080 GOT=CNT: EXPECT=0: DESC$="FOR 1 TO 5 STEP -1 executes 0 times": GOSUB 900

5090 CNT=0
5100 FOR I=5 TO 1 STEP 1
5110   CNT=CNT+1
5120 NEXT I
5130 GOT=CNT: EXPECT=0: DESC$="FOR 5 TO 1 STEP 1 executes 0 times": GOSUB 900
5140 RETURN

6000 REM ==========================================================
6010 REM TEST 6: NESTING + NEXT VARLIST + FINAL VALUE
6020 REM ==========================================================
6030 PRINT "TEST6: Nesting / NEXT varlist / final value"
6040 CNT=0: LASTI=0: LASTJ=0

6050 FOR I=1 TO 3
6060   FOR J=1 TO 2
6070     CNT=CNT+1
6080     LASTI=I: LASTJ=J
6090   NEXT J,I
6100 GOT=CNT: EXPECT=6: DESC$="Nested loops count 3*2=6 with NEXT J,I": GOSUB 900

6110 REM After completion, in GW-BASIC typically I has stepped past end
6120 REM For I=1 TO 3 STEP 1, final I is 4.
6130 GOT=I: EXPECT=4: DESC$="Loop var I after FOR I=1 TO 3 ends at 4": GOSUB 900

6140 REM J after inner loop J=1 TO 2 ends at 3, but after NEXT J,I it should still be 3
6150 GOT=J: EXPECT=3: DESC$="Loop var J after FOR J=1 TO 2 ends at 3": GOSUB 900

6160 RETURN
