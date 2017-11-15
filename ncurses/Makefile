ARGS = $(CPPARGS) -I ../
MILO_OBJECTS = $(addprefix ../, $(OBJECTS))

all: milo_ncurses

xterm:
	tic -x xterm-milo.nic

milo_ncurses: main.o $(MILO_OBJECTS)
	$(CXX) $(MILO_OBJECTS) main.o -o milo_ncurses -lncursesw

main.o: main.cpp ../ui.h
	$(CXX) $(ARGS) main.cpp -c

clean:
	rm -f milo_ncurses main.o

