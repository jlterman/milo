CPPARGS := $(CPPARGS) -I ../
OBJECTS := $(addprefix ../, $(OBJECTS))

all: milo_ncurses

xterm:
	tic -x xterm-milo.nic

milo_ncurses: main.o menu.o $(OBJECTS)
	$(CXX) $(OBJECTS) main.o menu.o -o milo_ncurses -lncursesw

main.o: main.cpp ../ui.h
	$(CXX) $(CPPARGS) main.cpp -c

menu.o: menu.cpp ../ui.h
	$(CXX) $(CPPARGS) menu.cpp -c

clean:
	rm -f milo_ncurses main.o menu.o

