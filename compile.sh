
clear && gcc -g -O0 -Wall -Wextra   wbasic_v1_11_phase1_ucase3.c   -o wbasic   $(pkg-config --cflags --libs gtk+-3.0)   -lm
