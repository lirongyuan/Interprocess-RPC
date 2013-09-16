Interprocess Remote Procedure Calls
===================================
Project link: http://www.cs.purdue.edu/homes/grr/cs354/lab3-rpc-smp/

Implemented a mechanism to call procedures that  are implemented in other processes.
A typicial use for this mechanism is a database that runs in a separate DB server
and that communicates with other DB clients.
The processes will use shared memory to communicate as well as
shared semaphores and mutexes for synchronization.

Setup:
------
```
make
```

Run the programs:
-----------------
```
BankServer global
or
BankServer account
```

```
BankClient [ 1 | 2 | 3 | 4 ]
```

