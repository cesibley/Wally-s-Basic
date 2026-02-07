10 REM ==========================================================
20 REM ARRAYS TORTURE TEST (GW-BASIC style)
30 REM Exercises:
40 REM  - Auto-dimensioning default (0..10)
50 REM  - Subscript out of range error behavior (ERR=9)
60 REM  - DIM 1D/2D and element access
70 REM  - Typed arrays ($, !, #) basics
80 REM  - OPTION BASE placement rule (must be before any array use)
90 REM ==========================================================

100 DEFINT A-Z
110 PASS=0: FAIL=0
120 ON ERROR GOTO 8000

130 PRINT "Running ARRAYS torture test..."
140 PRINT

150 GOSUB 1000
160 GOSUB 2000
170 GOSUB 3000
180 GOSUB 4000
190 GOSUB 5000

200 PRINT
210 PRINT "=================================="
220 PRINT "ARRAYS TORTURE TEST COMPLETE"
230 PRINT "PASS=";PASS;" FAIL=";FAIL
240 PRINT "=================================="
250 END

900 REM ---------------- ASSERT NUMERIC ----------------
910 IF GOT=EXPECT THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL:";DESC$;" EXPECT=";EXPECT;" GOT=";GOT
920 RETURN

930 REM ---------------- ASSERT STRING -----------------
940 IF GOT$=EXPECT$ THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL:";DESC$;" EXPECT=[";EXPECT$;"] GOT=[";GOT$;"]"
950 RETURN

960 REM ---------------- EXPECT ERROR (ERR=EXPECT) ------
970 EXP_MODE=1: EXP_ERR=EXPECT
980 RETURN

1000 REM ==========================================================
1010 REM TEST1: AUTO-DIM DEFAULT SIZE (GW-BASIC: 0..10)
1020 REM ==========================================================
1030 PRINT "TEST1: Auto-dimension default bounds"

1040 REM First reference should auto-dim A() to 0..10
1050 A(10)=123
1060 GOT=A(10): EXPECT=123: DESC$="Auto-dim allows A(10)": GOSUB 910

1070 EXPECT=9: DESC$="Auto-dim A(11) => ERR 9"
1080 GOSUB 960
1090 A(11)=1
1100 EXP_MODE=0
1110 RETURN

2000 REM ==========================================================
2010 REM TEST2: DIM 1D EXPLICIT + DEFAULT LOWER BOUND (0)
2020 REM ==========================================================
2030 PRINT "TEST2: DIM 1D"

2040 DIM B(5)
2050 B(0)=7: B(5)=9
2060 GOT=B(0): EXPECT=7: DESC$="DIM B(5): B(0) valid": GOSUB 910
2070 GOT=B(5): EXPECT=9: DESC$="DIM B(5): B(5) valid": GOSUB 910

2080 EXPECT=9: DESC$="DIM B(5): B(6) => ERR 9"
2090 GOSUB 960
2100 B(6)=1
2110 EXP_MODE=0
2120 RETURN

3000 REM ==========================================================
3010 REM TEST3: DIM 2D + INDEXING
3020 REM ==========================================================
3030 PRINT "TEST3: DIM 2D"

3040 DIM C(2,3)
3050 C(0,0)=11
3060 C(2,3)=99
3070 GOT=C(0,0): EXPECT=11: DESC$="C(0,0)=11": GOSUB 910
3080 GOT=C(2,3): EXPECT=99: DESC$="C(2,3)=99": GOSUB 910

3090 EXPECT=9: DESC$="C(3,0) => ERR 9"
3100 GOSUB 960
3110 C(3,0)=1
3120 EXP_MODE=0
3130 RETURN

4000 REM ==========================================================
4010 REM TEST4: TYPED ARRAYS ($, !, #)
4020 REM ==========================================================
4030 PRINT "TEST4: Typed arrays"

4040 DIM S$(2)
4050 S$(0)="HI": S$(2)="THERE"
4060 GOT$=S$(0): EXPECT$="HI": DESC$="String array S$(0)": GOSUB 940
4070 GOT$=S$(2): EXPECT$="THERE": DESC$="String array S$(2)": GOSUB 940

4080 DIM F!(1)
4090 F!(0)=1.5!: F!(1)=2.25!
4100 GOT=INT(F!(0)*100+0.5): EXPECT=150: DESC$="Single array F!(0)=1.5": GOSUB 910
4110 GOT=INT(F!(1)*100+0.5): EXPECT=225: DESC$="Single array F!(1)=2.25": GOSUB 910

4120 DIM D#(1)
4130 D#(1)=3.25#
4140 GOT=INT(D#(1)*100+0.5): EXPECT=325: DESC$="Double array D#(1)=3.25": GOSUB 910

4150 RETURN

5000 REM ==========================================================
5010 REM TEST5: OPTION BASE behavior after arrays already exist
5020 REM Accept either:
5030 REM  (A) GW-BASIC style: error if arrays already in use
5040 REM  (B) WBASIC style: allow, but must NOT retroactively change arrays
5050 REM ==========================================================

5060 PRINT "TEST5: OPTION BASE after arrays exist"

5070 REM Ensure an array already exists
5080 DIM L(3)
5090 L(0)=5
5100 L(3)=9

5110 REM Try OPTION BASE late
5120 ON ERROR GOTO 5200
5130 OPTION BASE 1
5140 ON ERROR GOTO 8000
5150 GOTO 5300

5200 REM ------------------------------------------
5210 REM Case A: OPTION BASE errored (GW-BASIC style)
5220 REM ------------------------------------------
5230 ON ERROR GOTO 8000
5240 PASS=PASS+1
5250 PRINT "NOTE: OPTION BASE after arrays caused ERR=";ERR;" (GW-BASIC-style)."
5260 ERR=0
5270 RETURN

5300 REM ------------------------------------------
5310 REM Case B: OPTION BASE allowed (WBASIC style)
5320 REM Verify existing array unaffected
5330 REM ------------------------------------------

5340 ON ERROR GOTO 5400
5350 T=L(0)
5360 ON ERROR GOTO 8000
5370 GOT=T: EXPECT=5: DESC$="Late OPTION BASE does not affect existing L(0)": GOSUB 910

5380 REM Verify new arrays use base 1
5390 DIM N(3)
5400 N(1)=42
5410 GOT=N(1): EXPECT=42: DESC$="New DIM after OPTION BASE uses base 1": GOSUB 910

5420 ON ERROR GOTO 5500
5430 N(0)=1
5440 ON ERROR GOTO 8000
5450 FAIL=FAIL+1
5460 PRINT "FAIL: After OPTION BASE 1, N(0) should be out of range"
5470 RETURN

5500 ON ERROR GOTO 8000
5510 IF ERR=9 THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: Expected ERR=9 for N(0), got ERR=";ERR
5520 ERR=0
5530 RETURN


8000 REM ==========================================================
8010 REM ERROR HANDLER
8020 REM ==========================================================
8030 IF EXP_MODE=0 THEN PRINT "UNEXPECTED ERROR: ERR=";ERR;" ERL=";ERL: FAIL=FAIL+1: RESUME 9999

8040 REM EXP_MODE=1 => exact expected error code (used for bounds tests)
8050 IF EXP_MODE=1 THEN
8060   GOT=ERR: EXPECT=EXP_ERR: GOSUB 910
8070   EXP_MODE=0
8080   RESUME NEXT
8090 END IF

8100 REM EXP_MODE=3 => OPTION BASE placement: accept syntax/illegal-use style errors
8110 IF EXP_MODE=3 THEN
8120   IF ERR=1 OR ERR=2 THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL:";DESC$;" expected ERR 1/2 got ERR=";ERR
8130   EXP_MODE=0
8140   RESUME NEXT
8150 END IF

9999 ON ERROR GOTO 0
10000 END
