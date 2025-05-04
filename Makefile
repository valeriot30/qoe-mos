CC = gcc
CFLAGS = -Wall -Wextra -g
OBJ = main.o buffer.o evaluation.o timer.o utils.o ./lib/json/cJSON.o ./lib/ws/ws.o ./lib/ws/utf8.o ./lib/ws/sha1.o ./lib/ws/handshake.o ./lib/ws/base64.o

main: $(OBJ)
	$(CC) $(CFLAGS) -o main $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o main