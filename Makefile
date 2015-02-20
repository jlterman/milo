OBJECTS = parser.o milo.o symbol.o xml.o
ARGS    = -g -std=c++11

all: milo_test milo_ncurses

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

milo.o: milo.cpp milo.h nodes.h milo_key.h
	g++ $(ARGS) milo.cpp -c

symbol.o: symbol.cpp milo.h nodes.h
	g++ $(ARGS) symbol.cpp -c

xml.o: xml.cpp xml.h
	g++ $(ARGS) xml.cpp -c

milo_key.h: genkey.sh
	sh genkey.sh

test: test.o
	g++ $(ARGS) test.o -o test

test.o: test.cpp
	g++ $(ARGS) test.cpp -c

clean:
	rm -f test milo_test milo_ncurses milo_key.h *.o
