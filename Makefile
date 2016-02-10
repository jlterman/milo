OBJECTS = parser.o nodes.o milo.o ui.o symbol.o xml.o
CPPARGS = -std=c++11 -Wall -Wextra -Werror -Wpedantic

all: debug

exe: milo_test milo_ncurses

debug: ARGS = $(CPPARGS) -g -D DEBUG
debug: exe

release: ARGS = $(CPPARGS) -O2
release: exe

unit: 
	cd unit_tests; make all

xterm:
	tic -x xterm-milo.nic

milo_ncurses: milo_ncurses.o $(OBJECTS)
	g++ $(OBJECTS) milo_ncurses.o -o milo_ncurses -lncursesw

milo_ncurses.o: milo_ncurses.cpp ui.h
	g++ $(ARGS) milo_ncurses.cpp -c

milo_test: milo_test.o $(OBJECTS)
	g++ $(ARGS) $(OBJECTS) milo_test.o -o milo_test

milo_test.o: milo_test.cpp ui.h
	g++ $(ARGS) milo_test.cpp -c

parser.o: parser.cpp milo.h util.h nodes.h
	g++ $(ARGS) parser.cpp -c

nodes.o: nodes.cpp milo.h ui.h util.h nodes.h
	g++ $(ARGS) nodes.cpp -c

milo.o: milo.cpp milo.h util.h
	g++ $(ARGS) milo.cpp -c

symbol.o: symbol.cpp milo.h util.h nodes.h
	g++ $(ARGS) symbol.cpp -c

xml.o: xml.cpp xml.h util.h
	g++ $(ARGS) xml.cpp -c

ui.o: ui.cpp ui.h milo.h util.h
	g++ $(ARGS) ui.cpp -c

test: test.o
	g++ test.o -o test

test.o: test.cpp
	g++ $(CPPARGS) -g test.cpp -c

clean:
	rm -f test milo_test milo_ncurses *.o
