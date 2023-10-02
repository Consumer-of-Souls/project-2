CC = cc
CFLAGS = -std=c11 -Wall -Werror

mysync: mysync.o glob2regex.o
	$(CC) $(CFLAGS) -o $@ $^

mysync.o: mysync.c glob2regex.o
	$(CC) $(CFLAGS) -c $< -o $@

glob2regex.o: glob2regex.c
	$(CC) $(CFLAGS) -c $< -o $@