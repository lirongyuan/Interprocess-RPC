//
// CS354: Implementaiton of the interprocess RPC
//

#include "RPC.h"


RPCServer::RPCServer( char * sharedFileName, int maxAvailableThreads )
{
	int fd,size;
	// 1. Create a file sharedFileName for RPCShared of the size we need for maxAvailableThreads
	fd=open(sharedFileName,O_RDWR| O_CREAT|O_TRUNC,0660);
	if (fd < 0) {
  		perror("open");exit(1);
	}
	// 2. mmap this file using the MAP_SHARED flag so changes in memory will be done in the file.
	// See mmap man pages
	size=sizeof(RPCShared)+(maxAvailableThreads-1)*sizeof(RPCThread);
	lseek(fd, size, SEEK_SET);
	write(fd," ",1);
	
	_shared=(RPCShared *)mmap(NULL,size,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
	if((void*)_shared==MAP_FAILED) { 
	 	perror("mmap"); exit(1);
 	}
	// 3. Initialze semaphores and mutex. Use the USYNC_PROCESS flags since semaphores and mutexs
	//    will be used to synchroinize threads across processes.
	// 
	mutex_init(&(_shared->_mutex),USYNC_PROCESS, NULL);
	sema_init(&(_shared->_sem_available_threads),maxAvailableThreads,USYNC_PROCESS,NULL);
	
	_shared->_currentSimultaneousCalls=0;	
	_shared->_maxSimultaneousCalls=0;	
	_shared->_totalCalls=0;
	_shared->_maxAvailableThreads=maxAvailableThreads;
	
	// 4. For each entry in the array rpcThread also initialize the semaphores and entries.
	for(int i=0;i<maxAvailableThreads;i++){
		sema_init(&(_shared->_rpcThread[i]._sem_request),0,USYNC_PROCESS,NULL);
		sema_init(&(_shared->_rpcThread[i]._sem_result),0,USYNC_PROCESS,NULL);
		_shared->_rpcThread[i]._in_use=false;
	}
}

void
RPCServer::registerRPC( char * name, RPCProcedure rpcProcedure, void * cookie )
{
	// Add an entry to the _rpcTable with name, rpcProcedure and cookie.
	RPCTableEntry rpc_entry;
	strcpy(rpc_entry._rpcName,name);
	rpc_entry._cookie=cookie;
	rpc_entry._rpcProcedure=rpcProcedure;
	
	mutex_lock(&(_shared->_mutex));
	
	_rpcTable[name]=rpc_entry;
	
	mutex_unlock(&(_shared->_mutex));
}

void
RPCServer::runServer()
{
	pthread_attr_t attr;
	pthread_attr_init( &attr );
	pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
	
	// 1. For each maxAvailableThreads-1 call pthread_create to
	// start a server thread calling threadServer  with the corresponding index 
	for(int i=0;i<_shared->_maxAvailableThreads-1;i++){
		pthread_t thr;
		
		ThreadServerArgs* thrArgs = new ThreadServerArgs(); 
		thrArgs->s = this;
		thrArgs->threadIndex= i+1;
		
		pthread_create( &thr, &attr, (void * (*)(void *))threadServer, (void *)thrArgs );
	}
	// 2. then call threadServer
	ThreadServerArgs* thrArgs =new ThreadServerArgs();
	thrArgs->s = this;
	thrArgs->threadIndex= 0;
	threadServer(thrArgs);
}


void 
RPCServer::threadServer( ThreadServerArgs * args ) 
{
	while (1) {
		// 1. wait for an incoming request for this thread
		RPCServer * server = args->s; // should this be outside ?
		RPCShared * shared = server->_shared;
		sema_wait(&(shared->_rpcThread[args->threadIndex]._sem_request));
		
		// 2. lookup the procedure in the rpctable
		RPCTableEntry* rpc_entry;
		rpc_entry = &(server->_rpcTable[shared->_rpcThread[args->threadIndex]._rpcName]);
		
		RPCProcedure procedure = rpc_entry->_rpcProcedure;
		
		// 3. Invoke procedure through a pointer
		(*procedure)(rpc_entry->_cookie,shared->_rpcThread[args->threadIndex]._argument,shared->_rpcThread[args->threadIndex]._result); 
		
		// 4. sema_post that the call is complete.
		sema_post(&(shared->_rpcThread[args->threadIndex]._sem_result)); 
	}
}

RPCClient::RPCClient( char * sharedFileName )
{
	// 1. Memory map the existing file
	int fd,size; 
	fd = open( sharedFileName , O_RDWR, 0660);
	if (fd < 0) {
  		perror("open");exit(1);
	}
	
	size = lseek(fd, 0, SEEK_END);
	
	mutex_lock(&(_shared->_mutex));
	_shared = (RPCShared *)mmap(NULL,size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	
	if ((void*)_shared==MAP_FAILED) { 
	 	perror("mmap"); exit(1);
 	}
 	mutex_unlock(&(_shared->_mutex));
}

int
RPCClient::call( char * rpcName, RPCArgument argument, RPCResult result )
{
	int index;
	// 1. Wait if there are no threads available
	sema_wait(&(_shared->_sem_available_threads));
	
	mutex_lock(&(_shared->_mutex));
	
	_shared->_currentSimultaneousCalls++;
	
	mutex_unlock(&(_shared->_mutex));
 	// 2. Get mutex
	mutex_lock(&(_shared->_mutex));
	
	// 3. Find the index of an available thread
	_shared->_totalCalls++;
	for(int i=0;i<_shared->_maxAvailableThreads;i++){
		if(_shared->_rpcThread[i]._in_use == false){
			index=i;
			_shared->_rpcThread[i]._in_use = true;
			break;
		}
	}
	// 4. release mutex
	mutex_unlock(&(_shared->_mutex));
	
	// 5. Copy argument into the RPCThread record for that index
	strcpy(_shared->_rpcThread[index]._rpcName,rpcName);
	memcpy(_shared->_rpcThread[index]._argument,argument,sizeof(RPCArgument));
	
	mutex_unlock(&(_shared->_mutex));
	// 6. Wake up that server thread 
	sema_post(&(_shared->_rpcThread[index]._sem_request));
	
	// 7. wait until results are ready.
	sema_wait(&(_shared->_rpcThread[index]._sem_result));
	
	mutex_lock(&(_shared->_mutex));
	
	memcpy(result,_shared->_rpcThread[index]._result,sizeof(RPCResult));

	_shared->_rpcThread[index]._in_use = false;
	_shared->_currentSimultaneousCalls--;
	
	mutex_unlock(&(_shared->_mutex));
	
	sema_post(&(_shared->_sem_available_threads));
	
	// 8. return
  return 0;
}

int 
RPCClient::maxSimultaneousCalls()
{
	if(_shared->_currentSimultaneousCalls > _shared->_maxSimultaneousCalls){
		_shared->_maxSimultaneousCalls=_shared->_currentSimultaneousCalls;
	}
	return _shared->_maxSimultaneousCalls;
}

int
RPCClient::totalCalls()
{
	return _shared->_totalCalls;
}




