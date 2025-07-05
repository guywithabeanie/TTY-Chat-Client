CC = g++
DEPEND = main.cpp io.cpp sockets.cpp networking.cpp
FLAGS = -g -Os -lncurses
EXE = tchat

main: $(DEPEND)
	g++ $(FLAGS) -o $(EXE) $(DEPEND)
