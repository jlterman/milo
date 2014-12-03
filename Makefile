all: milo_test milo_ncurses

milo_ncurses: parser.o milo.o milo_ncurses.o
	g++ -g -std=c++11 parser.o milo.o milo_ncurses.o -o milo_ncurses -lncurses

milo_ncurses.o: milo_ncurses.cpp milo.h
	g++ -g -std=c++11 milo_ncurses.cpp -c

milo_test: parser.o milo.o milo_test.o
	g++ -g -std=c++11 parser.o milo.o milo_test.o -o milo_test

milo_test.o: milo_test.cpp milo.h
	g++ -g -std=c++11 milo_test.cpp -c

parser.o: parser.cpp milo.h nodes.h
	g++ -g -std=c++11 parser.cpp -c

milo.o: milo.cpp milo.h nodes.h milo_key.h
	g++ -g -std=c++11 milo.cpp -c

milo_key.h: genkey.sh
	sh genkey.sh

test: test.o
	g++ -g -std=c++11 test.o -o test

test.o: test.cpp
	g++ -g -std=c++11 test.cpp -c

clean:
	rm -f test milo_test milo_ncurses milo_key.h *.o
