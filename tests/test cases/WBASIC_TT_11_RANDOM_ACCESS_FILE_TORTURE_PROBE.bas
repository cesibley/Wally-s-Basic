10 REM ==========================================================
20 REM RANDOM-ACCESS FILE I/O PROBE (GW-BASIC style)
30 REM Purpose:
40 REM  - Probe whether random-access file features exist (OPEN "R", FIELD, GET#, PUT#, LOF, SEEK)
50 REM  - If missing, ensure they fail cleanly (trap error) and do NOT corrupt normal sequential I/O
60 REM  - Always completes without stopping the interpreter
70 REM ==========================================================

100 DEFINT A-Z
110 PASS=0: FAIL=0

120 LAST_ERR=0: LAST_ERL=0
130 TRIPPED_ANY=0
140 LABEL$=""
150 ON ERROR GOTO 9000

160 PRINT "Running RANDOM-ACCESS FILE I/O probe..."
170 PRINT

180 SUP_OPENR=0
190 SUP_FIELD=0
200 SUP_GET=0
210 SUP_PUT=0
220 SUP_LOF=0
230 SUP_SEEK=0

240 GOSUB 1000
250 GOSUB 2000
260 GOSUB 3000
270 GOSUB 4000
280 GOSUB 5000

290 PRINT
300 PRINT "----------------------------------"
310 PRINT "RANDOM-ACCESS SUPPORT SUMMARY"
320 PRINT "OPEN ""R"" .......... ";SUP_OPENR
330 PRINT "FIELD ............. ";SUP_FIELD
340 PRINT "GET # ............. ";SUP_GET
350 PRINT "PUT # ............. ";SUP_PUT
360 PRINT "LOF(#) ............ ";SUP_LOF
370 PRINT "SEEK # ............ ";SUP_SEEK
380 PRINT "----------------------------------"

390 PRINT
400 PRINT "=================================="
410 PRINT "RANDOM-ACCESS FILE I/O PROBE COMPLETE"
420 PRINT "PASS=";PASS;" FAIL=";FAIL
430 PRINT "=================================="
440 END

900 REM ==========================================================
910 REM TRY helper:
920 REM   - set LABEL$ to what you're trying
930 REM   - execute exactly one statement at the TRYLINE
940 REM   - handler resumes next, then caller checks TRIPPED_ANY
950 REM ==========================================================
960 REM (no code here; see usage pattern)

1000 REM ==========================================================
1010 REM TEST1: OPEN "R" basic probe
1020 REM ==========================================================
1030 PRINT "TEST1: OPEN ""R"" probe"

1040 REM Try OPEN "R" with LEN=
1050 LABEL$="OPEN ""R"" AS #1 LEN=16"
1060 TRIPPED_ANY=0: LAST_ERR=0: LAST_ERL=0
1070 OPEN "ra_probe.tmp" FOR RANDOM AS #1 LEN=16
1080 IF TRIPPED_ANY=0 THEN SUP_OPENR=1: PASS=PASS+1: PRINT " OK: ";LABEL$ ELSE PASS=PASS+1: PRINT " ERR: ";LABEL$;"  ERR=";LAST_ERR;" ERL=";LAST_ERL
1090 IF SUP_OPENR=1 THEN CLOSE #1

1100 REM Try OPEN "R" without LEN (GW-BASIC usually requires LEN)
1110 LABEL$="OPEN ""R"" AS #1 (no LEN)"
1120 TRIPPED_ANY=0: LAST_ERR=0: LAST_ERL=0
1130 OPEN "ra_probe.tmp" FOR RANDOM AS #1
1140 IF TRIPPED_ANY=0 THEN PASS=PASS+1: PRINT " OK: ";LABEL$ ELSE PASS=PASS+1: PRINT " ERR: ";LABEL$;"  ERR=";LAST_ERR;" ERL=";LAST_ERL
1150 IF TRIPPED_ANY=0 THEN CLOSE #1

1160 RETURN

2000 REM ==========================================================
2010 REM TEST2: FIELD / LSET / RSET probe
2020 REM ==========================================================
2030 PRINT "TEST2: FIELD / LSET / RSET probe"

2040 IF SUP_OPENR=0 THEN PASS=PASS+1: PRINT " SKIP: OPEN ""R"" not supported, cannot FIELD": RETURN

2050 LABEL$="OPEN ""R"" AS #1 LEN=16 (for FIELD tests)"
2060 TRIPPED_ANY=0: LAST_ERR=0: LAST_ERL=0
2070 OPEN "ra_probe.tmp" FOR RANDOM AS #1 LEN=16
2080 IF TRIPPED_ANY<>0 THEN PASS=PASS+1: PRINT " ERR: ";LABEL$;"  ERR=";LAST_ERR;" ERL=";LAST_ERL: RETURN

2090 REM FIELD statement (expected missing in many interpreters)
2100 LABEL$="FIELD #1, 5 AS A$, 5 AS B$, 6 AS C$"
2110 TRIPPED_ANY=0: LAST_ERR=0: LAST_ERL=0
2120 FIELD #1, 5 AS A$, 5 AS B$, 6 AS C$
2130 IF TRIPPED_ANY=0 THEN SUP_FIELD=1: PASS=PASS+1: PRINT " OK: ";LABEL$ ELSE PASS=PASS+1: PRINT " ERR: ";LABEL$;"  ERR=";LAST_ERR;" ERL=";LAST_ERL

2140 IF SUP_FIELD=0 THEN CLOSE #1: RETURN

2150 REM If FIELD exists, check LSET/RSET don’t crash
2160 LABEL$="LSET A$=""HELLO"""
2170 TRIPPED_ANY=0: LAST_ERR=0: LAST_ERL=0
2180 LSET A$="HELLO"
2190 IF TRIPPED_ANY=0 THEN PASS=PASS+1: PRINT " OK: ";LABEL$ ELSE PASS=PASS+1: PRINT " ERR: ";LABEL$;"  ERR=";LAST_ERR;" ERL=";LAST_ERL

2200 LABEL$="RSET B$=""X"""
2210 TRIPPED_ANY=0: LAST_ERR=0: LAST_ERL=0
2220 RSET B$="X"
2230 IF TRIPPED_ANY=0 THEN PASS=PASS+1: PRINT " OK: ";LABEL$ ELSE PASS=PASS+1: PRINT " ERR: ";LABEL$;"  ERR=";LAST_ERR;" ERL=";LAST_ERL

2240 CLOSE #1
2250 RETURN

3000 REM ==========================================================
3010 REM TEST3: PUT # / GET # probe
3020 REM ==========================================================
3030 PRINT "TEST3: PUT # / GET # probe"

3040 IF SUP_OPENR=0 THEN PASS=PASS+1: PRINT " SKIP: OPEN ""R"" not supported, cannot GET/PUT": RETURN
3050 IF SUP_FIELD=0 THEN PASS=PASS+1: PRINT " NOTE: FIELD not supported; still probing GET/PUT presence"

3060 LABEL$="OPEN ""R"" AS #1 LEN=16 (for GET/PUT tests)"
3070 TRIPPED_ANY=0: LAST_ERR=0: LAST_ERL=0
3080 OPEN "ra_probe.tmp" FOR RANDOM AS #1 LEN=16
3090 IF TRIPPED_ANY<>0 THEN PASS=PASS+1: PRINT " ERR: ";LABEL$;"  ERR=";LAST_ERR;" ERL=";LAST_ERL: RETURN

3100 LABEL$="PUT #1, 1"
3110 TRIPPED_ANY=0: LAST_ERR=0: LAST_ERL=0
3120 PUT #1, 1
3130 IF TRIPPED_ANY=0 THEN SUP_PUT=1: PASS=PASS+1: PRINT " OK: ";LABEL$ ELSE PASS=PASS+1: PRINT " ERR: ";LABEL$;"  ERR=";LAST_ERR;" ERL=";LAST_ERL

3140 LABEL$="GET #1, 1"
3150 TRIPPED_ANY=0: LAST_ERR=0: LAST_ERL=0
3160 GET #1, 1
3170 IF TRIPPED_ANY=0 THEN SUP_GET=1: PASS=PASS+1: PRINT " OK: ";LABEL$ ELSE PASS=PASS+1: PRINT " ERR: ";LABEL$;"  ERR=";LAST_ERR;" ERL=";LAST_ERL

3180 CLOSE #1
3190 RETURN

4000 REM ==========================================================
4010 REM TEST4: LOF / SEEK probe
4020 REM ==========================================================
4030 PRINT "TEST4: LOF / SEEK probe"

4040 IF SUP_OPENR=0 THEN PASS=PASS+1: PRINT " SKIP: OPEN ""R"" not supported, cannot LOF/SEEK": RETURN

4050 LABEL$="OPEN ""R"" AS #1 LEN=16 (for LOF/SEEK tests)"
4060 TRIPPED_ANY=0: LAST_ERR=0: LAST_ERL=0
4070 OPEN "ra_probe.tmp" FOR RANDOM AS #1 LEN=16
4080 IF TRIPPED_ANY<>0 THEN PASS=PASS+1: PRINT " ERR: ";LABEL$;"  ERR=";LAST_ERR;" ERL=";LAST_ERL: RETURN

4090 LABEL$="X=LOF(1)"
4100 TRIPPED_ANY=0: LAST_ERR=0: LAST_ERL=0
4110 X=LOF(1)
4120 IF TRIPPED_ANY=0 THEN SUP_LOF=1: PASS=PASS+1: PRINT " OK: ";LABEL$;"  LOF=";X ELSE PASS=PASS+1: PRINT " ERR: ";LABEL$;"  ERR=";LAST_ERR;" ERL=";LAST_ERL

4130 LABEL$="SEEK #1, 1"
4140 TRIPPED_ANY=0: LAST_ERR=0: LAST_ERL=0
4150 SEEK #1, 1
4160 IF TRIPPED_ANY=0 THEN SUP_SEEK=1: PASS=PASS+1: PRINT " OK: ";LABEL$ ELSE PASS=PASS+1: PRINT " ERR: ";LABEL$;"  ERR=";LAST_ERR;" ERL=";LAST_ERL

4170 CLOSE #1
4180 RETURN

5000 REM ==========================================================
5010 REM TEST5: Sequential I/O sanity AFTER random-access probes
5020 REM ==========================================================
5030 PRINT "TEST5: Sequential I/O sanity after probes"

5040 REM Write a small sequential file
5050 LABEL$="OPEN FOR OUTPUT / PRINT #"
5060 TRIPPED_ANY=0: LAST_ERR=0: LAST_ERL=0
5070 OPEN "seq_probe.tmp" FOR OUTPUT AS #2
5080 PRINT #2, "HELLO"
5090 PRINT #2, "WORLD"
5100 CLOSE #2
5110 IF TRIPPED_ANY=0 THEN PASS=PASS+1: PRINT " OK: ";LABEL$ ELSE FAIL=FAIL+1: PRINT "FAIL:";LABEL$;"  ERR=";LAST_ERR;" ERL=";LAST_ERL

5120 REM Read it back with LINE INPUT #
5130 LABEL$="OPEN FOR INPUT / LINE INPUT #"
5140 TRIPPED_ANY=0: LAST_ERR=0: LAST_ERL=0
5150 OPEN "seq_probe.tmp" FOR INPUT AS #2
5160 LINE INPUT #2, A$
5170 LINE INPUT #2, B$
5180 CLOSE #2
5190 IF TRIPPED_ANY<>0 THEN FAIL=FAIL+1: PRINT "FAIL:";LABEL$;"  ERR=";LAST_ERR;" ERL=";LAST_ERL: RETURN

5200 IF A$="HELLO" THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: Seq read line1 expected [HELLO] got [";A$;"]"
5210 IF B$="WORLD" THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL: Seq read line2 expected [WORLD] got [";B$;"]"

5220 RETURN

9000 REM ==========================================================
9010 REM ERROR HANDLER
9020 REM ==========================================================
9030 TRIPPED_ANY=1
9040 LAST_ERR=ERR
9050 LAST_ERL=ERL
9060 ERR=0
9070 RESUME NEXT
