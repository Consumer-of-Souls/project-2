PROJECT = mysync
HEADERS = $(PROJECT).h
OBJ = mysync.o dirsync.o debugging.o manager.o lowlevels.o patterns.o filesync.o glob2regex.o

C11 = cc -std=c11
CFLAGS = -Wall -Werror

$(PROJECT): $(OBJ)
	$(C11) $(CFLAGS) -o $(PROJECT) $(OBJ) -lm

%.o: %.c $(HEADERS)
	$(C11) $(CFLAGS) -c $< -o $@

PHONY: clean
clean:
	rm -f $(PROJECT) $(OBJ)