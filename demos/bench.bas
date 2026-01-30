10 REM =========================================================
20 REM WBASIC BENCHMARK: INTERPRETED vs EXPORTED
30 REM
40 REM How to use:
50 REM  1) Run in interpreter, record times printed
60 REM  2) Export headless exe, run it, record times
70 REM
80 REM Notes:
90 REM  - Avoids MOD in warmup (WBASIC numeric edge) by using:
100 REM      I - INT(I/7)*7
110 REM  - Uses TIMER; results depend on system load
120 REM =========================================================
130 DEFINT A-D
140 CLS
150 PRINT "WBASIC Benchmark (Interpreted vs Exported)"
160 PRINT "----------------------------------------"
170 PRINT

180 GOSUB 9000

190 PRINT "Test 1: Integer arithmetic loop"
200 GOSUB 1000
210 PRINT

220 PRINT "Test 2: Floating math loop (SIN/COS/SQR)"
230 GOSUB 2000
240 PRINT

250 PRINT "Test 3: String concat + MID$"
260 GOSUB 3000
270 PRINT

280 PRINT "ALL DONE."
290 END

1000 REM ---------------------------------------------------------
1010 REM Test 1: Integer arithmetic (tight loop)
1020 REM ---------------------------------------------------------
1030 N = 500000
1040 A = 1: B = 2: C = 3: D = 4
1050 T0 = TIMER
1060 FOR I = 1 TO N
1070   A = A + B
1080   B = B + C
1090   C = C + D
1100   D = D + A
1110   IF A > 100000 THEN A = A - 100000
1120   IF B > 100000 THEN B = B - 100000
1130   IF C > 100000 THEN C = C - 100000
1140   IF D > 100000 THEN D = D - 100000
1150 NEXT I
1160 T1 = TIMER
1170 PRINT "  N="; N; "  seconds="; (T1 - T0)
1190 RETURN

2000 REM ---------------------------------------------------------
2010 REM Test 2: Floating math (SIN/COS/SQR)
2020 REM ---------------------------------------------------------
2030 N = 1000000
2040 X = 0.1
2050 Y = 0.0
2060 T0 = TIMER
2070 FOR I = 1 TO N
2080   X = X + 0.000001
2090   Y = Y + SIN(X) * COS(X) + SQR(X)
2100   IF X > 1000 THEN X = 0.1
2110 NEXT I
2120 T1 = TIMER
2130 PRINT "  N="; N; "  seconds="; (T1 - T0)
2150 RETURN

3000 REM ---------------------------------------------------------
3010 REM Test 3: String operations (concat + MID$)
3020 REM ---------------------------------------------------------
3030 N = 1500000
3040 S$ = ""
3050 T0 = TIMER
3060 FOR I = 1 TO N
3070   S$ = S$ + "A"
3080   IF LEN(S$) > 200 THEN S$ = MID$(S$, 101)
3090 NEXT I
3100 T1 = TIMER
3110 PRINT "  N="; N; "  seconds="; (T1 - T0)
3130 RETURN

9000 REM ---------------------------------------------------------
9010 REM Warmup: reduce one-time overhead differences
9020 REM MOD-free remainder: (I - INT(I/7)*7)
9030 REM ---------------------------------------------------------
9040 T = 200000
9050 Z = 0
9060 FOR I = 1 TO T
9070   Z = Z + (I - INT(I / 7) * 7)
9080 NEXT I
9090 RETURN
