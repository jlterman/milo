ARGS    = -g -std=c++11 -I ../

all: xml_test
	./xml_test

xml_test.o: xml_test.cpp ../xml.h
	g++ $(ARGS) xml_test.cpp  -c

xml.o: ../xml.cpp ../xml.h
	g++ $(ARGS) ../xml.cpp  -c

xml_test: xml_test.o xml.o
	g++ xml_test.o xml.o -o xml_test

