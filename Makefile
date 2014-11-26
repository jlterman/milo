all: milo_test milo_ncurses

milo_ncurses: milo.o milo_ncurses.o
	g++ -g -std=c++11 milo.o milo_ncurses.o -o milo_ncurses -lncurses

milo_ncurses.o: milo_ncurses.cpp milo.h
	g++ -g -std=c++11 milo_ncurses.cpp -c

milo_test: milo.o milo_test.o
	g++ -g -std=c++11 milo.o milo_test.o -o milo_test

milo_test.o: milo_test.cpp milo.h
	g++ -g -std=c++11 milo_test.cpp -c

milo.o: milo.cpp milo.h
	g++ -g -std=c++11 milo.cpp -c

test: test.o
	g++ -g -std=c++11 test.o -o test

test.o: test.cpp
	g++ -g -std=c++11 test.cpp -c

clean:
	rm -f test milo_test *.o
