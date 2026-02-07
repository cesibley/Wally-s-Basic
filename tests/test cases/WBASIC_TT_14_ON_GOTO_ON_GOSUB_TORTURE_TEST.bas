10 REM ==========================================================
20 REM ON ... GOTO / ON ... GOSUB TORTURE TEST (GW-BASIC style)
30 REM Exercises:
40 REM  - ON n GOTO branching + fallthrough (n out of range)
50 REM  - ON n GOSUB branching + return address correctness
60 REM  - n as expression
70 REM  - Statement chaining with ':' semantics
80 REM ==========================================================

100 DEFINT A-Z
110 PASS=0: FAIL=0
120 PRINT "Running ON GOTO / ON GOSUB torture test..."
130 PRINT

140 GOSUB 1000
150 GOSUB 2000
160 GOSUB 3000
170 GOSUB 4000
180 GOSUB 5000

190 PRINT
200 PRINT "=================================="
210 PRINT "ON GOTO / ON GOSUB TORTURE TEST COMPLETE"
220 PRINT "PASS=";PASS;" FAIL=";FAIL
230 PRINT "=================================="
240 END

900 REM ==========================================================
910 REM ASSERT NUMERIC
920 REM ==========================================================
930 IF GOT=EXPECT THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL:";DESC$;" EXPECT=";EXPECT;" GOT=";GOT
940 RETURN

950 REM ==========================================================
960 REM ASSERT STRING
970 REM ==========================================================
980 IF GOT$=EXPECT$ THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL:";DESC$;" EXPECT=[";EXPECT$;"] GOT=[";GOT$;"]"
990 RETURN

1000 REM ==========================================================
1010 REM TEST1: ON n GOTO basic branching
1020 REM ==========================================================
1030 PRINT "TEST1: ON n GOTO basic branching"

1040 FOR I=1 TO 3
1050   HIT=0
1060   ON I GOTO 1200,1210,1220
1070   REM If we get here, no branch (should not for I=1..3)
1080   HIT=99
1090   GOTO 1230

1200 HIT=10: GOTO 1230
1210 HIT=20: GOTO 1230
1220 HIT=30: GOTO 1230

1230 GOT=HIT
1240 EXPECT=I*10
1250 DESC$="ON GOTO selects correct target for n=" + STR$(I)
1260 GOSUB 930
1270 NEXT I
1280 RETURN

2000 REM ==========================================================
2010 REM TEST2: ON n GOTO fallthrough when out of range
2020 REM ==========================================================
2030 PRINT "TEST2: ON n GOTO fallthrough"

2040 HIT=0
2050 N=0
2060 ON N GOTO 2200,2210,2220
2070 HIT=1
2080 GOTO 2230
2200 HIT=10: GOTO 2230
2210 HIT=20: GOTO 2230
2220 HIT=30: GOTO 2230
2230 GOT=HIT: EXPECT=1: DESC$="n=0 falls through (no branch)": GOSUB 930

2240 HIT=0
2250 N=4
2260 ON N GOTO 2200,2210,2220
2270 HIT=1
2280 GOT=HIT: EXPECT=1: DESC$="n>count falls through (no branch)": GOSUB 930

2290 HIT=0
2300 N=-2
2310 ON N GOTO 2200,2210,2220
2320 HIT=1
2330 GOT=HIT: EXPECT=1: DESC$="n<1 falls through (no branch)": GOSUB 930
2340 RETURN

3000 REM ==========================================================
3010 REM TEST3: ON expr GOTO with expression evaluation
3020 REM ==========================================================
3030 PRINT "TEST3: ON expr GOTO expression"

3040 HIT=0
3050 A=5: B=2
3060 ON (A-B) GOTO 3200,3210,3220,3230
3070 HIT=99
3080 GOTO 3240
3200 HIT=10: GOTO 3240
3210 HIT=20: GOTO 3240
3220 HIT=30: GOTO 3240
3230 HIT=40: GOTO 3240
3240 GOT=HIT: EXPECT=30: DESC$="ON (A-B) where A=5,B=2 selects 3rd target": GOSUB 930
3250 RETURN

4000 REM ==========================================================
4010 REM TEST4: ON n GOSUB basic + return address correctness
4020 REM ==========================================================
4030 PRINT "TEST4: ON n GOSUB basic + return"

4040 FOR I=1 TO 3
4050   LOG$=""
4060   AFTER=0
4070   ON I GOSUB 4300,4310,4320
4080   AFTER=1
4090   REM We must be here after the sub returns
4100   GOT=AFTER: EXPECT=1: DESC$="Returned to next statement after ON GOSUB n=" + STR$(I): GOSUB 930
4110   REM LOG$ should contain the selected tag
4120   GOT$=LOG$
4130   IF I=1 THEN EXPECT$="A"
4140   IF I=2 THEN EXPECT$="B"
4150   IF I=3 THEN EXPECT$="C"
4160   DESC$="ON GOSUB selected correct sub for n=" + STR$(I)
4170   GOSUB 950
4180 NEXT I
4190 RETURN

4300 LOG$="A": RETURN
4310 LOG$="B": RETURN
4320 LOG$="C": RETURN

5000 REM ==========================================================
5010 REM TEST5: ':' statement chaining semantics
5020 REM ==========================================================
5030 PRINT "TEST5: ':' chaining semantics"

5040 REM (A) ON GOTO should NOT execute later statements on same line
5050 X=0: N=2
5060 ON N GOTO 5200,5210,5220: X=9
5070 REM If branch taken correctly, X=9 should NOT run.
5080 GOTO 5230
5200 X=1: GOTO 5230
5210 X=2: GOTO 5230
5220 X=3: GOTO 5230
5230 GOT=X: EXPECT=2: DESC$="ON GOTO skips rest of line after branch": GOSUB 930

5240 REM (B) ON GOSUB should return and continue with remaining statements on line
5250 X=0: N=3
5260 ON N GOSUB 5400,5410,5420: X=9
5270 GOT=X: EXPECT=9: DESC$="ON GOSUB continues with rest of line after return": GOSUB 930
5280 GOT$=LOG$: EXPECT$="C": DESC$="ON GOSUB picked correct sub in ':' chain": GOSUB 950

5290 REM (C) ON GOSUB out of range falls through, still executes rest of line
5300 X=0: N=0: LOG$=""
5310 ON N GOSUB 5400,5410,5420: X=7
5320 GOT=X: EXPECT=7: DESC$="ON GOSUB out-of-range falls through": GOSUB 930
5330 GOT$=LOG$: EXPECT$="": DESC$="No sub called when out-of-range": GOSUB 950

5340 RETURN

5400 LOG$="A": RETURN
5410 LOG$="B": RETURN
5420 LOG$="C": RETURN
