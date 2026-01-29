
clear && gcc -g -O0 -Wall -Wextra   wbasic.c   -o wbasic   $(pkg-config --cflags --libs gtk+-3.0)   -lm
