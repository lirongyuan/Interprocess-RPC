////////////////////////////////////////////////////////////
// RPC.h: Implements inter-process Remote Procedure Calls //
// using memory shared across processes and semaphores    //
////////////////////////////////////////////////////////////

#include <synch.h>
#include <iostream>
#include <map>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <thread.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/mman.h>

//#include <unistd.h>

using namespace std;
class RPCServer;

////  ///////// Maximum Limits /////  ////////

// Maximum length of the procedure name
const int RPCMaxName = 256;

// Maximum size of the arguments and results of the procedures
const int RPCMaxArgumentsOrResults = 1024;

////////////// RPC Types //////////////////

// Type of an RPC argument
typedef char RPCArgument[ RPCMaxArgumentsOrResults ];

// Type of an RPC result
typedef char RPCResult[ RPCMaxArgumentsOrResults ];

// Contains all the information a service thread needs
class RPCThread {

	friend class RPCClient;
	friend class RPCServer;

	// True if this thread is currently in use by a client
	bool _in_use;

	// Server waits until a request takes place
	sema_t _sem_request;

	// The client waits on this semaphore until the result is ready
	sema_t _sem_result;

	// Name of the procedure that is being executed
	char _rpcName[ RPCMaxName ];

	// The arguments of the procedure are stored here
	RPCArgument _argument;

	// The results are stored here
	RPCResult _result;
};

// RPC Shared structure. It is stored in a file and memory mapped
// by the Clients and the Server
class RPCShared {
	friend class RPCClient;
	friend class RPCServer;
public:
	// Protects the shared data structures
	mutex_t _mutex;

	// The clients will wait on this semaphor if all the threads are in use.
	sema_t _sem_available_threads;

	// Current simultaneous calls
	int _currentSimultaneousCalls;

	// Total calls
	int _totalCalls;

	// Max simultaneous calls
	int _maxSimultaneousCalls;	

	// The total number of server threads
	int _maxAvailableThreads;
	
	// The array of rpc threads. The index of 1 is used to make sure this is an array.
	// However, there will be up to _maxAvailableThreads RPCThread records here.
	RPCThread _rpcThread[1];
};

/////////////// Client Side /////////////////////////

// RPC Client object for the clients
class RPCClient {
	// Points to the memory mapped RPCShared object
	RPCShared * _shared;
public:
	// Constructor. Takes as argument the name of the file where RPCShared object is stored
	// and maps it into memory. If the file does not exist or something
	// wrong happens, it will exit.
	RPCClient( char * sharedFileName);

	// Performs a remote procedure call. rpcName is the name of the procedure
	// argument is a buffer where the arguments are stored. Result is a
	// buffer where the results are placed. call returns 0 if the call was
	// successfull.
	int call( char * rpcName, RPCArgument argument, RPCResult result );

	// Maximum simultaneous calls in server
	int maxSimultaneousCalls();

	// Total Calls
	int totalCalls();
};

/////////////// Server Side /////////////////////////

// Definition of an RPC procedure. Cookie is a pointer to some private data structure
// argument is a buffer of chars, Result is a buffer of chars and points to where the
// result is placed. Returns 0 if correct.
typedef int (*RPCProcedure)( void * cookie, RPCArgument argument, RPCResult result );

// Entry of the RPC table of rpc procedures registerd in the server.
class RPCTableEntry {
	friend class RPCServer;

	// Name of the procedure
	char _rpcName[ RPCMaxName ];

	// Pointer to some private data of the procedure
	void * _cookie;

	// Pointer to the procedure
	RPCProcedure _rpcProcedure;
};

struct ThreadServerArgs {
  RPCServer * s;
  int threadIndex;
};

class RPCServer {
	// Points to the area in memory where the shared RPC object was mapped
	RPCShared * _shared;

	// Table with all exported rpcs that can be called in this server
	map<string, RPCTableEntry> _rpcTable;

public:

	// Constructor. sharedFileName is the name of the file that will be created 
	// to store the RPCShared object. It exits if something wrong happens
	RPCServer( char * sharedFileName, int maxAvailableThreads);

	// Registers an RPC in the rpc table. name is the name of the procedure.
	// rpcProcedure is the procedure invoked when a call with this name arrives.
	// cookie is a pointer to the private data.
	void registerRPC( char * name, RPCProcedure rpcProcedure, void * cookie );

	// Infinite loop to wait for incoming calls. It will never return. The 
	// server has to be killed with ctrl-c
	void runServer();

	// Thread that serves requests
	static void threadServer( ThreadServerArgs * args );

};



