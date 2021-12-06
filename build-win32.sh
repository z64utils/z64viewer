mkdir -p bin
i686-w64-mingw32.static-gcc -DZ64VIEWER_WANT_MAIN src/*.c -o bin/z64viewer.exe -I include -lm `i686-w64-mingw32.static-pkg-config --cflags --libs glfw3` -s -flto -Os -DNDEBUG

