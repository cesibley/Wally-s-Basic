10 SCREEN 2
20 CLS
30 PI = 3.14159
35 st = 1
40 AMPLITUDE = 50 ' Adjust wave height
50 FREQUENCY = 0.1 ' Adjust wave length
60 YOFFSET = 100 ' Adjust vertical position (center of screen)
70 XSTART = 0 ' Initial horizontal start point
80
90 ' Main animation loop
100 FOR X = 0 TO 639 step st
110 Y = INT(AMPLITUDE * SIN((X + XSTART) * FREQUENCY)) + YOFFSET
120 PSET (X, Y), 3 ' Draw point in color 3 (Cyan in SCREEN 2)
130 NEXT X
140
150 ' Small delay for visibility
160 rem FOR PAUSE = 1 TO 100000: NEXT PAUSE
170
180 ' Erase the wave by redrawing in the background color (0 - Black)
190 FOR X = 0 TO 639 step st
200 Y = INT(AMPLITUDE * SIN((X + XSTART) * FREQUENCY)) + YOFFSET
210 PSET (X, Y), 0
220 NEXT X
230
240 XSTART = XSTART + 10 ' Shift the wave horizontally
250 IF XSTART > 639 THEN XSTART = 0 ' Wrap around the screen
260 GOTO 100 ' Repeat the loop
