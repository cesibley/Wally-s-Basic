10 REM ==========================================================
20 REM MANDELBROT FRACTAL (GW-BASIC / WBASIC) - SCREEN 12
30 REM - Press any key to stop
40 REM - Renders every other scanline for speed (change STEPY)
50 REM ==========================================================
60 SCREEN 12
70 CLS
80 COLOR 15,0
90 LOCATE 1,1: PRINT "MANDELBROT (SCREEN 12)  -  Press any key to stop"

100 MAXIT = 60
110 STEPY = 1      ' 1 = full resolution (slower), 2 = faster, 3+ even faster

120 XMIN = -2.5
130 XMAX =  1.0
140 YMIN = -1.5
150 YMAX =  1.5

160 XW = 639
170 YH = 479

180 FOR PY = 0 TO YH STEP STEPY
190   IF INKEY$ <> "" THEN END
200   CY = YMIN + (YMAX - YMIN) * (PY / YH)

210   FOR PX = 0 TO XW
220     CX = XMIN + (XMAX - XMIN) * (PX / XW)

230     ZX = 0: ZY = 0
240     I = 0

250     WHILE (ZX*ZX + ZY*ZY) < 4 AND I < MAXIT
260       XT = ZX*ZX - ZY*ZY + CX
270       ZY = 2 * ZX * ZY + CY
280       ZX = XT
290       I = I + 1
300     WEND

310     IF I >= MAXIT THEN
320       C = 0
330     ELSE
340       C = (I MOD 15) + 1     ' cycle 1..15
350     END IF

360     PSET (PX, PY), C
370     IF STEPY > 1 THEN PSET (PX, PY + 1), C
380   NEXT PX
390 NEXT PY

400 LOCATE 2,1: PRINT "Done. Press any key."
410 WHILE INKEY$ = "":
