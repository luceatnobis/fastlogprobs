WARN = -Wall -Wextra
FLAGS = -march=native
LINK = -lpthread -lm # -ljansson
PROG = logprobs
OPT = -O3

INCLUDE = # ./jansson-2.10/src

all:
	$(CC) -I $(INCLUDE) $(OPT) $(FLAGS) $(WARN) $(PROG).c $(LINK) -o $(PROG)
debug:
	$(CC) -I $(INCLUDE) $(FLAGS) $(WARN) -ggdb3 $(PROG).c $(LINK) -o $(PROG)

clean:
	rm $(PROG)
