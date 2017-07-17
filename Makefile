CC=clang
CFLAGS=-Wall -Wextra -g3 -ansi
SRCS=src/parser.c src/main.c src/vm.c
OBJS=$(SRCS:.c=.o)
TARGET=atto

all:	$(TARGET)
	@echo Successfully built atto

$(TARGET):	$(OBJS)
	$(CC) -o $(TARGET) $(OBJS)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm $(OBJS) $(TARGET)
