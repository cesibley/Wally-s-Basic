SCREEN 12
CLS
RANDOMIZE TIMER
KEY OFF

' ----- Settings -----
levels = 10          ' depth: try 8..12
baseLen = 90         ' trunk length
shrink = .72         ' branch shrink factor
spread = .45         ' branch angle in radians (~26 degrees)
jitter = .08         ' random angle variation (0 for perfect symmetry)

trunkColor = 6
leafColor = 10

' stack arrays
DIM sx(40000), sy(40000), slen(40000), sang(40000), slev(40000)

' root at bottom center, angle PI/2 (up)
x0 = 320
y0 = 460
a0 = 1.5707963

' push initial branch
sp = 1
sx(sp) = x0
sy(sp) = y0
slen(sp) = baseLen
sang(sp) = a0
slev(sp) = levels

DrawLoop:
IF sp <= 0 THEN GOTO Done

' pop
x1 = sx(sp)
y1 = sy(sp)
L = slen(sp)
A = sang(sp)
lv = slev(sp)
sp = sp - 1

IF lv <= 0 THEN GOTO DrawLoop

' endpoint
x2 = x1 + L * COS(A)
y2 = y1 - L * SIN(A)

' draw color by depth
IF lv <= 2 THEN
    colr = leafColor
ELSE
    colr = trunkColor
END IF

LINE (x1, y1)-(x2, y2), colr

' push children
IF lv > 1 THEN
    nL = L * shrink

    ' right branch
    rA = A - spread + (RND - .5) * jitter
    sp = sp + 1
    sx(sp) = x2
    sy(sp) = y2
    slen(sp) = nL
    sang(sp) = rA
    slev(sp) = lv - 1

    ' left branch
    lA = A + spread + (RND - .5) * jitter
    sp = sp + 1
    sx(sp) = x2
    sy(sp) = y2
    slen(sp) = nL
    sang(sp) = lA
    slev(sp) = lv - 1
END IF

GOTO DrawLoop

Done:
LOCATE 1, 1
COLOR 14
PRINT "Fractal Tree  depth="; levels; "  (press any key)"
a$ = INPUT$(1)
END