CC = gcc
CFLAGS = -Wall -Wextra -g
OBJ = main.o buffer.o evaluation.o timer.o utils.o ./json/cJSON.o ./ws/ws.o ./ws/utf8.o ./ws/sha1.o ./ws/handshake.o ./ws/base64.o

main: $(OBJ)
	$(CC) $(CFLAGS) -o main $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o main