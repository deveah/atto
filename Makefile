CC=clang
SRCS=src/atto.c src/parser.c src/lexer.c src/state.c src/compiler.c src/vm.c
OBJS=$(SRCS:.c=.o)
CFLAGS=-Wall -Wextra -g3 -ansi -c
LIBS=-lreadline
TARGET=atto

all: $(SRCS) $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LIBS) $(OBJS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm $(TARGET) $(OBJS)
