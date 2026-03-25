REM ==========================================================
REM BOUNCY BALL - SCREEN 12
REM Speed + Angle physics + Screen Border
REM Press any key to exit
REM ==========================================================

DEFINT A-Z

SCREEN 12
WIDTH 80
CLS
RANDOMIZE TIMER

PI! = 3.14159265

REM ---- DRAW SCREEN BORDER ----
LINE (0, 0)-(639, 479), 15, B
LINE (1, 1)-(638, 478), 8, B

REM ---- BALL SIZE ----
R = 30
DIAM = R * 2
SPRW = DIAM + 1
SPRH = DIAM + 1

DIM B%(20000)

REM ---- INITIAL POSITION ----
X! = 200
Y! = 150

REM ---- SPEED + ANGLE ----
SPEEED! = 4.5
ANGLE! = RND * 2 * PI!

BALLC = 14
OUTLINEC = 15
BGC = 0

REM ---- CREATE SPRITE ----
TMPX = 10
TMPY = 10
LINE (TMPX, TMPY)-(TMPX + SPRW - 1, TMPY + SPRH - 1), BGC, BF
CIRCLE (TMPX + R, TMPY + R), R, OUTLINEC
PAINT (TMPX + R, TMPY + R), BALLC, OUTLINEC
GET (TMPX, TMPY)-(TMPX + SPRW - 1, TMPY + SPRH - 1), B%
LINE (TMPX, TMPY)-(TMPX + SPRW - 1, TMPY + SPRH - 1), BGC, BF

OLDX = INT(X!)
OLDY = INT(Y!)
PUT (OLDX, OLDY), B%, XOR

DO
    IF INKEY$ <> "" THEN EXIT DO

    PUT (OLDX, OLDY), B%, XOR

    VX! = SPEEED! * COS(ANGLE!)
    VY! = SPEEED! * SIN(ANGLE!)

    X! = X! + VX!
    Y! = Y! + VY!

    REM ---- LEFT / RIGHT WALL ----
    IF X! < 1 THEN
        X! = 1
        ANGLE! = PI! - ANGLE!
        ANGLE! = ANGLE! + (RND - .5) * .2
    END IF

    IF X! > 638 - (SPRW - 1) THEN
        X! = 638 - (SPRW - 1)
        ANGLE! = PI! - ANGLE!
        ANGLE! = ANGLE! + (RND - .5) * .2
    END IF

    REM ---- TOP / BOTTOM WALL ----
    IF Y! < 1 THEN
        Y! = 1
        ANGLE! = -ANGLE!
        ANGLE! = ANGLE! + (RND - .5) * .2
    END IF

    IF Y! > 478 - (SPRH - 1) THEN
        Y! = 478 - (SPRH - 1)
        ANGLE! = -ANGLE!
        ANGLE! = ANGLE! + (RND - .5) * .2
    END IF

    NEWX = INT(X!)
    NEWY = INT(Y!)

    PUT (NEWX, NEWY), B%, XOR
    OLDX = NEWX
    OLDY = NEWY

    FOR D = 1 TO 2000
    NEXT D
LOOP

PUT (OLDX, OLDY), B%, XOR
LOCATE 1, 1
PRINT "Done."
END