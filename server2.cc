
#include <stdio.h>
#include <thread.h>
#include <assert.h>
#include <unistd.h>

#include "RPC.h"

int add( void * cookie, RPCArgument argument, RPCResult result )
{
	struct Args {
	  int a;
	  int b;
	} * arg = (Args *) argument;

	struct Res {
		int c;
	} * res = (Res *) result;

	res->c = arg->a + arg->b;

	return 0;
}

int add_with_delay( void * cookie, RPCArgument argument, RPCResult result )
{
	struct Args {
	  int a;
	  int b;
	  int delay;
	} * arg = (Args *) argument;

	struct Res {
		int c;
	} * res = (Res *) result;

	// Sleep for about delay secs
	for ( int i = 0; i < 10 * arg->delay; i++ ) {
	  usleep( 100000 );
	}

	res->c = arg->a + arg->b;

	return 0;
}

int
main( int argc, char ** argv )
{
	RPCServer * server = new RPCServer( "server2.rpc", 5 );
	server->registerRPC( "add", add, 0 );
	server->registerRPC( "add_with_delay", add_with_delay, 0 );
	
	printf(" Server2 ready\n");

	server->runServer();
}




