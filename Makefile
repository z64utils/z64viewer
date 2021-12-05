.PHONY = z64viewer.exe clean

z64viewer.exe: src/*.c include/*.h
	i686-w64-mingw32.static-gcc src/*.c -o z64viewer.exe -I include -lm `i686-w64-mingw32.static-pkg-config --cflags --libs glfw3` -s -flto -Os -DNDEBUG

clean:
	rm -f z64viewer.exe