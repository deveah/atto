CC=clang
SRCS=src/atto.c src/parser.c src/lexer.c src/state.c
CFLAGS=-Wall -Wextra -g3 -ansi
LIBS=-lreadline
TARGET=atto

all: $(TARGET)
	@echo "Built Atto."

$(TARGET):
	$(CC) $(CFLAGS) $(LIBS) $(SRCS) -o $(TARGET)

clean:
	rm $(TARGET)

