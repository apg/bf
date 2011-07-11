CC = gcc
DCFLAGS = -g -DDEBUG=1
PCFLAGS = -pg

src = util.c bf.c main.c
objs = util.o bf.o main.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

bf: $(objs)
	$(CC) $(CFLAGS) -o $@ $^

debug:
	$(CC) $(DCFLAGS) -o bf-debug $(src)

profile:
	$(CC) $(PCFLAGS) -o bf-profile $(src)

clean:
	@$(RM) -rf *.o bf.dSYM