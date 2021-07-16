CC = gcc
CFLAGS = -c -MD -g -Wall
THREADFLAG = -pthread
INCLUDES = -I./include -I./include/Network -I./include/Integration
SRC = main.c src/common.c src/Network/worker.c src/Network/server.c src/Integration/calc.c src/Integration/pthread_manager.c
OBJ = $(SRC:.c=.o)
EXEC = calc.out

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(THREADFLAG) $(INCLUDES) -o $@ $^

%.o: %.c
	$(CC) $(THREADFLAG) $(CFLAGS) $(INCLUDES) -o $@ $<

.PHONY: clean
clean:
	rm -rf $(OBJ) $(OBJ:.o=.d)

-include $(OBJ:.o=.d)
