OBJECTS := parser.o nodes.o milo.o ui.o symbol.o xml.o
CPPARGS := -std=c++11 -Wall -Wextra -Werror -Wpedantic $(CFLAGS)
MAKE ?= make
export

all: debug

exe: milo_test milo_ncurses

debug: CPPARGS := $(CPPARGS) -g -D DEBUG
debug: exe

release: CPPARGS := $(CPPARGS) -O2
release: exe

unit_tests: FORCE
	$(MAKE) -C $@
FORCE:

milo_ncurses: $(OBJECTS) FORCE
	$(MAKE) -C ncurses

milo_test: milo_test.o $(OBJECTS)
	$(CXX) $(CPPARGS) $(OBJECTS) milo_test.o -o milo_test

milo_test.o: milo_test.cpp ui.h
	$(CXX) $(CPPARGS) milo_test.cpp -c

parser.o: parser.cpp milo.h util.h nodes.h
	$(CXX) $(CPPARGS) parser.cpp -c

nodes.o: nodes.cpp milo.h ui.h util.h nodes.h
	$(CXX) $(CPPARGS) nodes.cpp -c

milo.o: milo.cpp milo.h util.h
	$(CXX) $(CPPARGS) milo.cpp -c

symbol.o: symbol.cpp milo.h util.h nodes.h
	$(CXX) $(CPPARGS) symbol.cpp -c

xml.o: xml.cpp xml.h util.h
	$(CXX) $(CPPARGS) xml.cpp -c

ui.o: ui.cpp ui.h milo.h util.h
	$(CXX) $(CPPARGS) ui.cpp -c

test: test.o
	$(CXX) test.o -o test

test.o: test.cpp
	$(CXX) $(CPPARGS) -g test.cpp -c

clean:
	rm -f test milo_test *.o
	$(MAKE) -C ncurses clean
