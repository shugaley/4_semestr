CC = gcc
CFLAGS = -MD -g -Wall
OBJS = rb_tree.o unit_test.o


all: libRbTree.a


libRbTree.a: rb_tree.o
	ar rcs $@ $^


test: test.out

test.out: test.o 
	gcc -o $@ $^

test.o: unit_test.o libRbTree.a 
	ld -r -o $@ $^


%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $^


clean:
	rm -rf test.out libRbTree.a $(OBJS) $(OBJS:.o=.d)

-include $(OBJS:.o=.d)

.PHONY: clean


