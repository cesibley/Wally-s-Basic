10 REM ==========================================================
20 REM WBASIC / GW-BASIC IF TORTURE TEST (INCLUDES BLOCK IF)
30 REM Exercises:
40 REM  - Single-line IF / IF-THEN-ELSE
50 REM  - Block IF / ELSEIF / ELSE / END IF
60 REM  - Nested IFs
70 REM  - Relational ops, AND/OR/NOT, precedence
80 REM  - Numeric + string expressions
90 REM ==========================================================

100 DEFINT A-Z
110 PASS=0: FAIL=0
120 PRINT "Running IF torture test..."
130 PRINT

140 GOSUB 1000  ' Single-line IF tests
150 GOSUB 2000  ' Block IF basic tests
160 GOSUB 3000  ' Block IF + expressions
170 GOSUB 4000  ' ELSEIF chains
180 GOSUB 5000  ' Nested IF torture
190 GOSUB 6000  ' AND / OR / NOT precedence
200 GOSUB 7000  ' String comparisons

210 PRINT
220 PRINT "=================================="
230 PRINT "IF TORTURE TEST COMPLETE"
240 PRINT "PASS=";PASS;" FAIL=";FAIL
250 PRINT "=================================="
260 END

1000 REM ==================================================
1010 REM SINGLE-LINE IF / IF-THEN-ELSE
1020 REM ==================================================
1030 PRINT "Single-line IF tests"
1040 A=5: B=10

1050 EXPECT=1: IF A=5 THEN GOT=1 ELSE GOT=0: DESC$="IF A=5": GOSUB 9000
1060 EXPECT=0: IF A=6 THEN GOT=1 ELSE GOT=0: DESC$="IF A=6": GOSUB 9000
1070 EXPECT=1: IF A<B THEN GOT=1 ELSE GOT=0: DESC$="IF A<B": GOSUB 9000
1080 EXPECT=1: IF A+B=15 THEN GOT=1 ELSE GOT=0: DESC$="IF A+B=15": GOSUB 9000
1090 EXPECT=1: IF (A*2)+B=20 THEN GOT=1 ELSE GOT=0: DESC$="IF (A*2)+B=20": GOSUB 9000

1100 RETURN

2000 REM ==================================================
2010 REM BASIC BLOCK IF / ELSE / END IF
2020 REM ==================================================
2030 PRINT "Block IF basic tests"
2040 A=3

2050 GOT=0
2060 IF A=3 THEN
2070   GOT=1
2080 END IF
2090 EXPECT=1: DESC$="Block IF A=3": GOSUB 9000

2100 GOT=0
2110 IF A=4 THEN
2120   GOT=1
2130 ELSE
2140   GOT=2
2150 END IF
2160 EXPECT=2: DESC$="Block IF ELSE path": GOSUB 9000

2170 RETURN

3000 REM ==================================================
3010 REM BLOCK IF WITH EXPRESSIONS
3020 REM ==================================================
3030 PRINT "Block IF with expressions"
3040 X=4: Y=6

3050 GOT=0
3060 IF X+Y=10 THEN
3070   GOT=1
3080 END IF
3090 EXPECT=1: DESC$="Block IF X+Y=10": GOSUB 9000

3100 GOT=0
3110 IF (X*Y)=24 THEN
3120   GOT=1
3130 ELSE
3140   GOT=2
3150 END IF
3160 EXPECT=1: DESC$="Block IF (X*Y)=24": GOSUB 9000

3170 RETURN

4000 REM ==================================================
4010 REM ELSEIF CHAINS
4020 REM ==================================================
4030 PRINT "ELSEIF chain tests"
4040 A=7

4050 GOT=0
4060 IF A=5 THEN
4070   GOT=1
4080 ELSEIF A=6 THEN
4090   GOT=2
4100 ELSEIF A=7 THEN
4110   GOT=3
4120 ELSE
4130   GOT=4
4140 END IF
4150 EXPECT=3: DESC$="ELSEIF selects correct branch": GOSUB 9000

4160 RETURN

5000 REM ==================================================
5010 REM NESTED IF TORTURE
5020 REM ==================================================
5030 PRINT "Nested IF torture"
5040 A=5: B=10

5050 GOT=0
5060 IF A=5 THEN
5070   IF B=10 THEN
5080     GOT=1
5090   ELSE
5100     GOT=2
5110   END IF
5120 ELSE
5130   GOT=3
5140 END IF
5150 EXPECT=1: DESC$="Nested IF true/true": GOSUB 9000

5160 RETURN

6000 REM ==================================================
6010 REM AND / OR / NOT + PRECEDENCE
6020 REM ==================================================
6030 PRINT "AND / OR / NOT precedence"
6040 A=5: B=10

6050 GOT=0
6060 IF A=5 AND B=10 THEN
6070   GOT=1
6080 END IF
6090 EXPECT=1: DESC$="A=5 AND B=10": GOSUB 9000

6100 GOT=0
6110 IF A=5 OR B=5 THEN
6120   GOT=1
6130 END IF
6140 EXPECT=1: DESC$="A=5 OR B=5": GOSUB 9000

6150 GOT=0
6160 IF NOT(A=6) THEN
6170   GOT=1
6180 END IF
6190 EXPECT=1: DESC$="NOT(A=6)": GOSUB 9000

6200 GOT=0
6210 IF A=6 OR A=5 AND B=10 THEN
6220   GOT=1
6230 END IF
6240 EXPECT=1: DESC$="AND precedence over OR": GOSUB 9000

6250 RETURN

7000 REM ==================================================
7010 REM STRING COMPARISONS
7020 REM ==================================================
7030 PRINT "String IF tests"
7040 A$="HELLO": B$="WORLD"

7050 GOT=0
7060 IF A$="HELLO" THEN
7070   GOT=1
7080 END IF
7090 EXPECT=1: DESC$="A$=""HELLO""": GOSUB 9000

7100 GOT=0
7110 IF A$<>B$ THEN
7120   GOT=1
7130 END IF
7140 EXPECT=1: DESC$="A$<>B$": GOSUB 9000

7150 GOT=0
7160 IF A$<B$ THEN
7170   GOT=1
7180 END IF
7190 EXPECT=1: DESC$="String < comparison": GOSUB 9000

7200 RETURN

9000 REM ==================================================
9010 REM RESULT CHECK
9020 REM ==================================================
9030 IF GOT=EXPECT THEN
9040   PASS=PASS+1
9050 ELSE
9060   FAIL=FAIL+1
9070   PRINT "FAIL:";DESC$;" EXPECT=";EXPECT;" GOT=";GOT
9080 END IF
9090 RETURN
