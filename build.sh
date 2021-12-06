mkdir -p bin
gcc -DZ64VIEWER_WANT_MAIN src/*.c -o bin/z64viewer -I include -lm -lglfw -ldl

