CC = gcc
CFLAGS = -g

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

bf: util.o bf.o
	$(CC) $(CFLAGS) -o $@ $^

clean:
	@$(RM) -rf *.o bf.dSYM