CC = gcc
LD = gcc

DIR_BUILD = build

EXEC = $(addprefix $(DIR_BUILD)/,calc.out)
FLAGS = -MD -g -Wall -pthread
CFLAGS = $(FLAGS)
LFLAGS = $(FLAGS)
OBJS = $(addprefix $(DIR_BUILD)/,Integration.o main.o)

all: $(EXEC)

$(EXEC): $(OBJS)
	$(LD) $(LFLAGS) -o $@ $^

$(OBJS) : | dirs

$(DIR_BUILD)/%.o : %.c
	$(CC) -c $(CFLAGS) -o $@ $<

-include $(OBJS:.o=.d)

.PHONY: dirs
dirs:
	mkdir -p $(DIR_BUILD)

.PHONY: clean
clean:
	rm -rf $(DIR_BUILD)