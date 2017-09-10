p4:  main.cpp shell.cpp shell.h
	g++ -std=c++0x -o shell main.cpp shell.cpp -lrt
debug:  main.cpp shell.cpp shell.h
	g++ -g -std=c++0x -o shell_dbg main.cpp shell.cpp -lrt
clean:
	rm -f shell *.o

