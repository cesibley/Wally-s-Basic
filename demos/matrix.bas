10 ' WBASIC: Matrix Rain (80x25, strict + headless friendly)
15 speed 85
20 RANDOMIZE TIMER
30 CLS
40 COLOR 2,8: CLS

50 W = 80
60 H = 25

70 ' Number of streams (density)
80 N = 55

90 ' Throttle (bigger = slower, smaller = faster)
100 DELAY = 700

110 DIM X(N), Y(N), L(N), S(N)

120 ' Initialize streams
130 FOR I = 1 TO N
140   X(I) = INT(RND * W) + 1
150   Y(I) = INT(RND * H) + 1
160   L(I) = INT(RND * 14) + 6
170   S(I) = INT(RND * 3) + 1
180 NEXT I

190 K$ = ""

200 ' ===== MAIN LOOP =====
210 IF K$ = CHR$(27) THEN GOTO 900

220 ' Occasional clear
230 REM IF RND < 0.002 THEN COLOR 2,0: CLS

240 FOR I = 1 TO N

250   ' Probabilistic speed
260   IF RND >= (0.35 / S(I)) THEN GOTO 520

270   ' Erase tail
280   TY = Y(I) - L(I)
290   IF TY < 1 THEN TY = TY + H
300   LOCATE TY, X(I)
310   COLOR 0,0
320   PRINT " ";

330   ' Trail body (dim green)
340   BY = Y(I) - 1
350   IF BY < 1 THEN BY = BY + H
360   LOCATE BY, X(I)
370   COLOR 2,0
380   R = INT(RND * 44)
390   IF R < 26 THEN C$ = CHR$(65 + R) ELSE IF R < 36 THEN C$ = CHR$(48 + (R - 26)) ELSE C$ = MID$("!@#$%^&*+-=?/", (R - 36) + 1, 1)
400   PRINT C$;

410   ' Head (bright green)
420   LOCATE Y(I), X(I)
430   COLOR 10,0
440   R = INT(RND * 44)
450   IF R < 26 THEN C$ = CHR$(65 + R) ELSE IF R < 36 THEN C$ = CHR$(48 + (R - 26)) ELSE C$ = MID$("!@#$%^&*+-=?/", (R - 36) + 1, 1)
460   PRINT C$;

470   ' Advance head
480   Y(I) = Y(I) + 1
490   IF Y(I) > H THEN Y(I) = 1

500   ' Occasionally change stream params
510   IF RND < 0.010 THEN X(I) = INT(RND * W) + 1
515   IF RND < 0.010 THEN L(I) = INT(RND * 14) + 6
518   IF RND < 0.006 THEN S(I) = INT(RND * 3) + 1

520 NEXT I

530 ' Throttle once per frame
540 FOR D = 1 TO DELAY: NEXT D

550 ' Exit on ESC
560 K$ = INKEY$
570 GOTO 210

900 COLOR 7,0
910 CLS
920 PRINT "Done."
