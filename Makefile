CFLAGS=-Wall -Werror
SEALER=./sealer

$(SEALER): sealer.c
	$(CC) $(CFLAGS) $< -o $(SEALER)

run: $(SEALER)
	$(SEALER) -f ./libskiko-linux-x64.so
