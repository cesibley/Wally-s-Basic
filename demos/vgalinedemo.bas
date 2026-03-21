10 screen 12:cls
20 color 15
30 locate 13,20
40 print "Press Any Key to Cycle Color. Q to Quit"
50 for C=1 to 15
60 locate 14,36
70 print "Color ";c
80 for n=0 to 48
90 line (0,n*10) - (n*13.33,479), c
100 line (n*13.33,479) - (639,479-n*10), c
110 line (639,479-n*10) - (639-n*13.33,0), c
120 line (639-n*13.33,0) - (0,n*10), c
130 next n
140 a$=inkey$
150 if a$="Q" then end
160 if a$="" then 140
170 next c
180 goto 10
