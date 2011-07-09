CC = gcc
DCFLAGS = -g
PCFLAGS = -pg

objs = util.o bf.o main.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

bf: $(objs)
	$(CC) $(CFLAGS) -o $@ $^

debug: $(objs)
	$(CC) $(DEBUGCFLAGS) -o bf-debug $^

profile: $(objs)
	$(CC) $(PCFLAGS) -o bf-profile $^

clean:
	@$(RM) -rf *.o bf.dSYM