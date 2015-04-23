OBJECTS = parser.o milo.o symbol.o xml.o
CPPARGS = -std=c++11 -Wall -Wextra -Werror -Wpedantic

all: debug

exe: milo_test milo_ncurses

debug: ARGS = $(CPPARGS) -g -D DEBUG
debug: exe

release: ARGS = $(CPPARGS) -O2
release: exe

unit: 
	cd unit_tests; make all

milo_ncurses: milo_ncurses.o $(OBJECTS)
	g++ $(OBJECTS) milo_ncurses.o -o milo_ncurses -lncurses

milo_ncurses.o: milo_ncurses.cpp milo.h
	g++ $(ARGS) milo_ncurses.cpp -c

milo_test: milo_test.o $(OBJECTS)
	g++ $(ARGS) $(OBJECTS) milo_test.o -o milo_test

milo_test.o: milo_test.cpp milo.h
	g++ $(ARGS) milo_test.cpp -c

parser.o: parser.cpp milo.h nodes.h
	g++ $(ARGS) parser.cpp -c

milo.o: milo_key.h milo.cpp milo.h nodes.h
	g++ $(ARGS) milo.cpp -c

symbol.o: symbol.cpp milo.h nodes.h
	g++ $(ARGS) symbol.cpp -c

xml.o: xml.cpp xml.h
	g++ $(ARGS) xml.cpp -c

milo_key.h: genkey.sh
	sh genkey.sh

test: test.o
	g++ test.o -o test

test.o: test.cpp
	g++ $(CPPARGS) -g test.cpp -c

clean:
	rm -f test milo_test milo_ncurses milo_key.h *.o
