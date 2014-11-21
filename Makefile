all: milo

milo: milo.o
	g++ -g -std=c++11 milo.o -o milo

milo.o: milo.cpp
	g++ -g -std=c++11 milo.cpp -c

test: test.o
	g++ -g -std=c++11 test.o -o test

test.o: test.cpp
	g++ -g -std=c++11 test.cpp -c

clean:
	rm -f test milo *.o
