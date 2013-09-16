
CC = g++ -g -D_REENTRANT

all: client1 server1 client2 server2 BankClient BankServer 

RPC.o: RPC.cc RPC.h
	$(CC) -c RPC.cc

client1.o: client1.cc RPC.h
	$(CC) -c client1.cc

client1: client1.o RPC.o
	$(CC) -o client1 client1.o RPC.o -lthread

server1.o: server1.cc RPC.h
	$(CC) -c server1.cc

server1: server1.o RPC.o
	$(CC) -o server1 server1.o RPC.o -lthread

BankClient.o: BankClient.cc RPC.h
	$(CC) -c BankClient.cc

BankClient: BankClient.o RPC.o
	$(CC) -o BankClient BankClient.o RPC.o -lthread

BankServer.o: BankServer.cc RPC.h
	$(CC) -c BankServer.cc

BankServer: BankServer.o RPC.o
	$(CC) -o BankServer BankServer.o RPC.o -lthread

server2: server2.cc RPC.o
	$(CC) -o server2 server2.cc RPC.o -lthread

client2: client2.cc RPC.o
	$(CC) -o client2 client2.cc RPC.o -lthread

clean:
	rm -f *.o client1 server1 client2 server2 BankServer BankClient *.rpc *~



