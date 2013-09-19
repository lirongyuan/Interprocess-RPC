Interprocess Remote Procedure Calls
===================================

A mechanism to call procedures that are implemented in other processes. A typicial use for this mechanism is a database that runs in a separate DB server
and that communicates with other DB clients. The processes will use shared memory to communicate as well as shared semaphores and mutexes for synchronization.

http://www.cs.purdue.edu/homes/grr/cs354/lab3-rpc-smp/

Download
--------
```
$ git clone https://github.com/lirongyuan/Interprocess-RPC.git
```

Setup
-----
```
$ make
```

Usage
-----
Compile your program with RPC.o and lthread library.
```
$ g++ -o prog prog.o RPC.o -lthread
```

Tests
-----
Server
```
$ ./BankServer [global | account]
```

Client
```
$ ./BankClient [1 | 2 | 3 | 4]
```


