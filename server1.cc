
#include <stdio.h>

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

int
main( int argv, char ** argc )
{
	RPCServer * server = new RPCServer( "server1.rpc", 1 );
	server->registerRPC( "add", add, 0 );
	
	printf(" Server ready\n");

	server->runServer();
}
