10 REM ==========================================================
20 REM INPUT / LINE INPUT / WRITE TORTURE TEST (GW-BASIC style)
30 REM Automated (no keyboard input required)
40 REM Exercises:
50 REM  - WRITE # -> INPUT # round-trip (strings/nums/quotes/commas)
60 REM  - PRINT # -> LINE INPUT # raw line fidelity
70 REM  - Blank numeric field from INPUT # (should become 0)
80 REM ==========================================================

100 DEFINT A-Z
110 PASS=0: FAIL=0
120 ON ERROR GOTO 8000

130 FN$="input_torture_tmp.txt"
140 PRINT "Running INPUT/LINE INPUT/WRITE torture test..."
150 PRINT

160 GOSUB 1000
170 GOSUB 2000
180 GOSUB 3000

190 PRINT
200 PRINT "=================================="
210 PRINT "INPUT/LINE INPUT/WRITE TORTURE TEST COMPLETE"
220 PRINT "PASS=";PASS;" FAIL=";FAIL
230 PRINT "=================================="
240 END

900 REM ---------------- ASSERT NUMERIC ----------------
910 IF GOT=EXPECT THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL:";DESC$;" EXPECT=";EXPECT;" GOT=";GOT
920 RETURN

930 REM ---------------- ASSERT STRING -----------------
940 IF GOT$=EXPECT$ THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL:";DESC$;" EXPECT=[";EXPECT$;"] GOT=[";GOT$;"]"
950 RETURN

960 REM ---------------- ASSERT DOUBLE (tolerant) -------
970 REM Compare doubles by scaling to integer thousandths
980 IF INT(GOTD*1000+0.5)=INT(EXPECTD*1000+0.5) THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL:";DESC$;" EXPECT~=";EXPECTD;" GOT=";GOTD
990 RETURN

1000 REM ==========================================================
1010 REM TEST1: WRITE # -> INPUT # ROUND-TRIP
1020 REM ==========================================================
1030 PRINT "TEST1: WRITE # -> INPUT # round-trip"

1040 A$="A,B"
1050 N=12
1060 X#=3.25
1070 Q$="HE said ""WOW"""

1080 OPEN FN$ FOR OUTPUT AS #1
1090 WRITE #1, A$, N, X#, Q$
1100 CLOSE #1

1110 OPEN FN$ FOR INPUT AS #1
1120 INPUT #1, B$, M, Y#, R$
1130 CLOSE #1

1140 GOT$=B$: EXPECT$=A$: DESC$="WRITE/INPUT string with comma": GOSUB 930
1150 GOT=M:  EXPECT=N:  DESC$="WRITE/INPUT integer": GOSUB 900
1160 GOTD=Y#: EXPECTD=X#: DESC$="WRITE/INPUT double 3.25": GOSUB 960
1170 GOT$=R$: EXPECT$=Q$: DESC$="WRITE/INPUT embedded quotes": GOSUB 930

1180 RETURN

2000 REM ==========================================================
2010 REM TEST2: PRINT # -> LINE INPUT # RAW LINE FIDELITY
2020 REM ==========================================================
2030 PRINT "TEST2: PRINT # -> LINE INPUT # raw fidelity"

2040 OPEN FN$ FOR OUTPUT AS #1
2050 PRINT #1, "LINE1"
2060 PRINT #1, "A,B,C"
2070 PRINT #1, "HE said ""WOW"""
2080 PRINT #1, "[  LEAD+TRAIL  ]"
2090 CLOSE #1

2100 OPEN FN$ FOR INPUT AS #1
2110 LINE INPUT #1, L1$
2120 LINE INPUT #1, L2$
2130 LINE INPUT #1, L3$
2140 LINE INPUT #1, L4$
2150 CLOSE #1

2160 GOT$=L1$: EXPECT$="LINE1": DESC$="LINE INPUT # line1": GOSUB 930
2170 GOT$=L2$: EXPECT$="A,B,C": DESC$="LINE INPUT # preserves commas": GOSUB 930
2180 GOT$=L3$: EXPECT$="HE said ""WOW""": DESC$="LINE INPUT # preserves quotes": GOSUB 930
2190 GOT$=L4$: EXPECT$="[  LEAD+TRAIL  ]": DESC$="LINE INPUT # preserves spaces": GOSUB 930

2200 RETURN

3000 REM ==========================================================
3010 REM TEST3: INPUT # BLANK FIELDS (STRING + NUMERIC)
3020 REM ==========================================================
3030 PRINT "TEST3: INPUT # blank fields"

3040 OPEN FN$ FOR OUTPUT AS #1
3050 PRINT #1, CHR$(34); CHR$(34); ",,"
3060 CLOSE #1

3070 OPEN FN$ FOR INPUT AS #1
3080 INPUT #1, S$, T$, Z
3090 CLOSE #1

3100 GOT$=S$: EXPECT$="": DESC$="Blank quoted string reads empty": GOSUB 930
3110 GOT$=T$: EXPECT$="": DESC$="Blank string field reads empty": GOSUB 930
3120 GOT=Z:  EXPECT=0:  DESC$="Blank numeric field reads 0": GOSUB 900

3130 RETURN

8000 REM ==========================================================
8010 REM ERROR HANDLER
8020 REM ==========================================================
8030 PRINT "ERROR TRAP: ERR=";ERR;" ERL=";ERL
8040 FAIL=FAIL+1
8050 RESUME 8060
8060 ON ERROR GOTO 0
8070 END
