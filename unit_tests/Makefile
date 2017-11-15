ARGS = $(CPPARGS) -g -I ../

all: clean xml_test
	./xml_test
	./milo_test.sh

xml_test.o: xml_test.cpp ../xml.h
	$(CXX) $(ARGS) xml_test.cpp  -c

xml.o: ../xml.cpp ../xml.h
	$(CXX) $(ARGS) ../xml.cpp  -c

xml_test: xml_test.o xml.o
	$(CXX) xml_test.o xml.o -o xml_test

clean:
	rm -f *.o xml_test
