.PHONY: all clean

# compiler name
CC=g++

# place options like -O2, -g here
CFLAGS= -O3 -ldl -lpthread -pthread -std=c++14

piepie-server: server.cpp 
	$(CC) -o piepie-server server.cpp $(CFLAGS)
	$(CC) -o piepie-client client.cpp $(CFLAGS)

clean:
	rm -f piepie-server
	rm -f piepie-client