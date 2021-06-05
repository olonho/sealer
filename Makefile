CFLAGS=-Wall -Werror -static
SEALER=./sealer

$(SEALER): sealer.c
	$(CC) $(CFLAGS) $< -o $(SEALER)

run: $(SEALER)
	$(SEALER) -f ./libskiko-linux-x64.so

clean:
	rm -f *.o $(SEALER)
