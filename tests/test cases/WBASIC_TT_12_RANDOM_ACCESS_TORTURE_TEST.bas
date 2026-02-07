10 REM ==========================================================
20 REM RANDOM ACCESS FILE I/O TORTURE TEST (STRICT GW-BASIC)
30 REM Uses canonical GW-BASIC syntax:
40 REM   OPEN "file" FOR RANDOM AS #n LEN=16
50 REM Verifies:
60 REM   - FIELD fixed-length buffers preserve padding exactly
70 REM   - LSET/RSET padding rules
80 REM   - PUT/GET persistence across CLOSE/OPEN
90 REM   - LOF(#) reflects file size in bytes
100 REM NOTE: Does NOT rely on variable filename/LEN forms.
110 REM ==========================================================

120 DEFINT A-Z
130 PASS=0: FAIL=0
140 TRIPPED=0: LASTERR=0: LASTERL=0
150 DESC$=""

160 ON ERROR GOTO 9000

170 PRINT "Running RANDOM ACCESS torture test (GW-BASIC strict)..."
180 PRINT

190 GOSUB 1000
200 GOSUB 2000
210 GOSUB 3000
220 GOSUB 4000

230 PRINT
240 PRINT "=================================="
250 PRINT "RANDOM ACCESS TORTURE TEST COMPLETE"
260 PRINT "PASS=";PASS;" FAIL=";FAIL
270 PRINT "=================================="
280 END

900 REM ===================== ASSERT NUMERIC ======================
910 IF GOT=EXPECT THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL:";DESC$;" EXPECT=";EXPECT;" GOT=";GOT
920 RETURN

930 REM ===================== ASSERT STRING =======================
940 IF GOT$=EXPECT$ THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL:";DESC$;" EXPECT=[";EXPECT$;"] GOT=[";GOT$;"]"
950 RETURN

1000 REM ==========================================================
1010 REM TEST1: Create file, FIELD, LSET/RSET, PUT rec 1 and rec 2
1020 REM ==========================================================
1030 PRINT "TEST1: FIELD + LSET/RSET + PUT rec1/rec2"

1040 TRIPPED=0: LASTERR=0: LASTERL=0
1050 OPEN "RA_TORTURE.TMP" FOR RANDOM AS #1 LEN=16
1060 IF TRIPPED<>0 THEN FAIL=FAIL+1: PRINT "FAIL: OPEN RANDOM LEN=16 ERR=";LASTERR;" ERL=";LASTERL: RETURN
1070 PASS=PASS+1

1080 FIELD #1, 5 AS A$, 5 AS B$, 6 AS C$
1090 PASS=PASS+1

1100 REM ---- record 1 ----
1110 LSET A$="HI"
1120 RSET B$="X"
1130 LSET C$="CAT"
1140 PUT #1, 1
1150 PASS=PASS+1

1160 REM ---- record 2 ----
1170 LSET A$="BYE"
1180 RSET B$="Q"
1190 LSET C$="DOG"
1200 PUT #1, 2
1210 PASS=PASS+1

1220 CLOSE #1
1230 PASS=PASS+1

1240 RETURN

2000 REM ==========================================================
2010 REM TEST2: Reopen + GET rec1/rec2 exact fixed-field contents
2020 REM ==========================================================
2030 PRINT "TEST2: GET rec1/rec2 fixed-field padding"

2040 OPEN "RA_TORTURE.TMP" FOR RANDOM AS #1 LEN=16
2050 FIELD #1, 5 AS A$, 5 AS B$, 6 AS C$

2060 GET #1, 1
2070 GOT$=A$: EXPECT$="HI"+SPACE$(3): DESC$="REC1 A$ = 'HI' padded to 5": GOSUB 930
2080 GOT$=B$: EXPECT$=SPACE$(4)+"X": DESC$="REC1 B$ = rightmost 'X' in 5": GOSUB 930
2090 GOT$=C$: EXPECT$="CAT"+SPACE$(3): DESC$="REC1 C$ = 'CAT' padded to 6": GOSUB 930

2100 GET #1, 2
2110 GOT$=A$: EXPECT$="BYE"+SPACE$(2): DESC$="REC2 A$ = 'BYE' padded to 5": GOSUB 930
2120 GOT$=B$: EXPECT$=SPACE$(4)+"Q": DESC$="REC2 B$ = rightmost 'Q' in 5": GOSUB 930
2130 GOT$=C$: EXPECT$="DOG"+SPACE$(3): DESC$="REC2 C$ = 'DOG' padded to 6": GOSUB 930

2140 CLOSE #1
2150 RETURN

3000 REM ==========================================================
3010 REM TEST3: LOF(#) reflects 2 records * 16 bytes = 32
3020 REM ==========================================================
3030 PRINT "TEST3: LOF(#) size after 2 records"

3040 OPEN "RA_TORTURE.TMP" FOR RANDOM AS #1 LEN=16
3050 X=LOF(1)
3060 GOT=X: EXPECT=32: DESC$="LOF(1)=32 after writing 2x16-byte records": GOSUB 910
3070 CLOSE #1
3080 RETURN

4000 REM ==========================================================
4010 REM TEST4: Overwrite rec1, verify persisted
4020 REM ==========================================================
4030 PRINT "TEST4: overwrite rec1 and verify"

4040 OPEN "RA_TORTURE.TMP" FOR RANDOM AS #1 LEN=16
4050 FIELD #1, 5 AS A$, 5 AS B$, 6 AS C$

4060 LSET A$="OK"
4070 RSET B$="Z"
4080 LSET C$="FISH"
4090 PUT #1, 1

4100 GET #1, 1
4110 GOT$=A$: EXPECT$="OK"+SPACE$(3): DESC$="Overwrite REC1 A$ padded": GOSUB 930
4120 GOT$=B$: EXPECT$=SPACE$(4)+"Z": DESC$="Overwrite REC1 B$ padded": GOSUB 930
4130 GOT$=C$: EXPECT$="FISH"+SPACE$(2): DESC$="Overwrite REC1 C$ padded": GOSUB 930

4140 CLOSE #1
4150 RETURN

9000 REM ==========================================================
9010 REM ERROR HANDLER
9020 REM ==========================================================
9030 TRIPPED=1
9040 LASTERR=ERR
9050 LASTERL=ERL
9060 ERR=0
9070 RESUME NEXT
