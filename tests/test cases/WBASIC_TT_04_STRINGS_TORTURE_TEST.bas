10 REM ==========================================================
20 REM STRINGS TORTURE TEST (GW-BASIC style)
30 REM Exercises:
40 REM  - LEN, LEFT$, RIGHT$, MID$ (forms)
50 REM  - INSTR (2-arg and 3-arg forms)
60 REM  - CHR$, ASC
70 REM  - STRING$, SPACE$
80 REM  - String concatenation (+) and comparisons
90 REM  - MID$ assignment (valid + expected error cases)
100 REM  - UCASE$, LCASE$
110 REM ==========================================================

120 DEFINT A-Z
130 PASS=0: FAIL=0
140 ON ERROR GOTO 8000

150 PRINT "Running STRINGS torture test..."
160 PRINT

170 GOSUB 1000
180 GOSUB 2000
190 GOSUB 3000
200 GOSUB 4000
210 GOSUB 5000
220 GOSUB 6000
230 GOSUB 7000

240 PRINT
250 PRINT "=================================="
260 PRINT "STRINGS TORTURE TEST COMPLETE"
270 PRINT "PASS=";PASS;" FAIL=";FAIL
280 PRINT "=================================="
290 END

900 REM ---------------- ASSERT NUMERIC ----------------
910 IF GOT=EXPECT THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL:";DESC$;" EXPECT=";EXPECT;" GOT=";GOT
920 RETURN

930 REM ---------------- ASSERT STRING -----------------
940 IF GOT$=EXPECT$ THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL:";DESC$;" EXPECT=[";EXPECT$;"] GOT=[";GOT$;"]"
950 RETURN

960 REM ---------------- EXPECT ERROR (any ERR) --------
970 EXP_MODE=1: EXP_ERL=EXPECT
980 RETURN

1000 REM ==========================================================
1010 REM TEST1: BASIC STRING FUNCTIONS
1020 REM ==========================================================
1030 PRINT "TEST1: LEN/LEFT$/RIGHT$/MID$"

1040 GOT=LEN("HELLO"): EXPECT=5: DESC$="LEN(HELLO)=5": GOSUB 910
1050 GOT$=LEFT$("HELLO",2): EXPECT$="HE": DESC$="LEFT$(HELLO,2)=HE": GOSUB 930
1060 GOT$=RIGHT$("HELLO",3): EXPECT$="LLO": DESC$="RIGHT$(HELLO,3)=LLO": GOSUB 930
1070 GOT$=MID$("HELLO",2,3): EXPECT$="ELL": DESC$="MID$(HELLO,2,3)=ELL": GOSUB 930
1080 GOT$=MID$("HELLO",4): EXPECT$="LO": DESC$="MID$(HELLO,4)=LO": GOSUB 930

1090 RETURN

2000 REM ==========================================================
2010 REM TEST2: INSTR FORMS
2020 REM ==========================================================
2030 PRINT "TEST2: INSTR"

2040 GOT=INSTR("BANANA","ANA"): EXPECT=2: DESC$="INSTR(BANANA,ANA)=2": GOSUB 910
2050 GOT=INSTR(3,"BANANA","ANA"): EXPECT=4: DESC$="INSTR(3,BANANA,ANA)=4": GOSUB 910
2060 GOT=INSTR("HELLO","X"): EXPECT=0: DESC$="INSTR(HELLO,X)=0": GOSUB 910

2070 RETURN

3000 REM ==========================================================
3010 REM TEST3: CHR$ / ASC
3020 REM ==========================================================
3030 PRINT "TEST3: CHR$/ASC"

3040 GOT=ASC("A"): EXPECT=65: DESC$="ASC(A)=65": GOSUB 910
3050 GOT$=CHR$(65): EXPECT$="A": DESC$="CHR$(65)=A": GOSUB 930
3060 GOT=ASC(CHR$(200)): EXPECT=200: DESC$="ASC(CHR$(200))=200": GOSUB 910

3070 RETURN

4000 REM ==========================================================
4010 REM TEST4: STRING$ / SPACE$ + CONCAT
4020 REM ==========================================================
4030 PRINT "TEST4: STRING$/SPACE$/concat"

4040 GOT$=STRING$(5,"*"): EXPECT$="*****": DESC$="STRING$(5,""*"" )": GOSUB 930
4050 GOT$=SPACE$(3): EXPECT$="   ": DESC$="SPACE$(3)=3 spaces": GOSUB 930
4060 GOT$="AB"+"CD": EXPECT$="ABCD": DESC$="String concatenation with +": GOSUB 930

4070 RETURN

5000 REM ==========================================================
5010 REM TEST5: STRING COMPARISONS (TRUE=-1 FALSE=0)
5020 REM ==========================================================
5030 PRINT "TEST5: Comparisons"

5040 GOT=("A"="A"): EXPECT=-1: DESC$="""A""=""A"" is TRUE (-1)": GOSUB 910
5050 GOT=("A"<"B"): EXPECT=-1: DESC$="""A""<""B"" is TRUE (-1)": GOSUB 910
5060 GOT=("B"<"A"): EXPECT=0: DESC$="""B""<""A"" is FALSE (0)": GOSUB 910
5070 GOT=("A">"B"): EXPECT=0: DESC$="""A"">""B"" is FALSE (0)": GOSUB 910

5080 RETURN

6000 REM ==========================================================
6010 REM TEST6: MID$ ASSIGNMENT (valid + expected errors)
6020 REM ==========================================================
6030 PRINT "TEST6: MID$ assignment"

6040 A$="ABCDE"
6050 MID$(A$,2,2)="zz"
6060 GOT$=A$: EXPECT$="AzzDE": DESC$="MID$(A$,2,2)=""zz""": GOSUB 930

6070 A$="ABCDE"
6080 MID$(A$,5)="!"
6090 GOT$=A$: EXPECT$="ABCD!": DESC$="MID$(A$,5)=""!""": GOSUB 930

6100 REM --- Expected error: position beyond LEN should error ---
6110 DESC$="MID$(A$,7,1)=... should error (out of range)"
6120 EXP_MODE=1: EXP_ERL=6130: TRIPPED=0
6130 A$="ABCDE": MID$(A$,7,1)="X"
6140 IF TRIPPED=0 THEN FAIL=FAIL+1: PRINT "FAIL:";DESC$;" (no error)" ELSE PASS=PASS+1

6150 REM --- Expected error: cannot assign into a literal ---
6160 DESC$="MID$(literal,...) assignment should error"
6170 EXP_MODE=1: EXP_ERL=6190: TRIPPED=0
6190 MID$("ABCDE",2,1)="X"
6200 IF TRIPPED=0 THEN FAIL=FAIL+1: PRINT "FAIL:";DESC$;" (no error)" ELSE PASS=PASS+1

6210 RETURN


7000 REM ==========================================================
7010 REM TEST7: UCASE$ / LCASE$
7020 REM ==========================================================
7030 PRINT "TEST7: UCASE$/LCASE$"

7040 GOT$=UCASE$("aBc"): EXPECT$="ABC": DESC$="UCASE$(aBc)=ABC": GOSUB 930
7050 GOT$=LCASE$("aBc"): EXPECT$="abc": DESC$="LCASE$(aBc)=abc": GOSUB 930

7060 RETURN

8000 REM ==========================================================
8010 REM ERROR HANDLER
8020 REM ==========================================================
8030 IF EXP_MODE=0 THEN PRINT "UNEXPECTED ERROR: ERR=";ERR;" ERL=";ERL: FAIL=FAIL+1: RESUME 9999

8040 REM Expected error: count PASS if ERL matches the expected line
8050 TRIPPED=1
8060 IF ERL=EXP_ERL THEN PASS=PASS+1 ELSE FAIL=FAIL+1: PRINT "FAIL:";DESC$;" expected ERL=";EXP_ERL;" got ERL=";ERL;" (ERR=";ERR;")"
8070 EXP_MODE=0
8080 ERR=0
8090 RESUME NEXT

9999 ON ERROR GOTO 0
10000 END
