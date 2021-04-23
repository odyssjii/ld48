emcc -std=c11 -O3 code/ld48.c -s USE_SDL=2 -s USE_SDL_TTF=2 -s ALLOW_MEMORY_GROWTH=1  -o build/ld48.html --embed-file build/novem___.ttf@novem___.ttf --shell-file code/minimal_shell.html 
