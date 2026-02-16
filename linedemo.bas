10 screen 1:cls
20 RANDOMIZE TIMER
30 MIN = 1
40 MAX = 16
70 C = INT(RND * (MAX - MIN + 1) + MIN)
80 for n=0 to 20
90 line (0,n*10) - (n*16,199), c
100 line (n*16,199) - (319,199-n*10), c
110 line (319,199-n*10) - (319-n*16,0), c
120 line (319-n*16,0) - (0,n*10), c
130 next n
