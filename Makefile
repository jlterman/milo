OBJECTS = parser.o nodes.o milo.o ui.o symbol.o xml.o
CPPARGS = -std=c++11 -Wall -Wextra -Werror -Wpedantic $(CFLAGS)
MAKE ?= make
export

all: debug

exe: milo_test milo_ncurses

debug: ARGS = $(CPPARGS) -g -D DEBUG
debug: exe

release: ARGS = $(CPPARGS) -O2
release: exe

unit_tests: FORCE
	$(MAKE) -C $@
FORCE:

xterm:
	tic -x xterm-milo.nic

milo_ncurses: milo_ncurses.o $(OBJECTS)
	$(CXX) $(OBJECTS) milo_ncurses.o -o milo_ncurses -lncursesw

milo_ncurses.o: milo_ncurses.cpp ui.h
	$(CXX) $(ARGS) milo_ncurses.cpp -c

milo_test: milo_test.o $(OBJECTS)
	$(CXX) $(ARGS) $(OBJECTS) milo_test.o -o milo_test

milo_test.o: milo_test.cpp ui.h
	$(CXX) $(ARGS) milo_test.cpp -c

parser.o: parser.cpp milo.h util.h nodes.h
	$(CXX) $(ARGS) parser.cpp -c

nodes.o: nodes.cpp milo.h ui.h util.h nodes.h
	$(CXX) $(ARGS) nodes.cpp -c

milo.o: milo.cpp milo.h util.h
	$(CXX) $(ARGS) milo.cpp -c

symbol.o: symbol.cpp milo.h util.h nodes.h
	$(CXX) $(ARGS) symbol.cpp -c

xml.o: xml.cpp xml.h util.h
	$(CXX) $(ARGS) xml.cpp -c

ui.o: ui.cpp ui.h milo.h util.h
	$(CXX) $(ARGS) ui.cpp -c

test: test.o
	$(CXX) test.o -o test

test.o: test.cpp
	$(CXX) $(CPPARGS) -g test.cpp -c

clean:
	rm -f test milo_test milo_ncurses *.o
