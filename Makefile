# Protocoale de comunicatii:
# Laborator 8: Multiplexare
# Makefile

CFLAGS =

# Portul pe care asculta serverul (de completat)
PORT = 12345

# Adresa IP a serverului (de completat)
IP_SERVER = 127.0.0.1

all: server subscriber

# Compileaza server.cpp
server: server.cpp

# Compileaza subscriber.cpp 
subscriber: subscriber.cpp

.PHONY: clean run_server run_client

# Ruleaza serverul
run_server:
	./server ${PORT}

# Ruleaza clientul
run_client:
	./subscriber ${IP_SERVER} ${PORT}

clean:
	rm -f server subscriber
