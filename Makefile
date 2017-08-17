CC=clang
CFLAGS=-Wall -Wextra -g3 -ansi

all: attoc attoi
	@echo "Successfully built the Atto tools (attoc, attoi)"

attoc:
	$(CC) $(CFLAGS) src/attoc.c -o attoc

attoi:
	$(CC) $(CFLAGS) src/attoi.c -o attoi

clean:
	rm attoc attoi

