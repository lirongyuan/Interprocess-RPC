
#include <stdio.h>

#include "RPC.h"

int
main( int argv, char ** argc )
{
	RPCClient * client = new RPCClient( "server1.rpc" );

	RPCArgument argBuffer;
	RPCResult resultBuffer;

	struct Args {
		int a;
		int b;
	} * arg = (Args *) argBuffer;

	struct Res {
		int c;
	} * res = (Res *) resultBuffer;

	arg->a = 5;
	arg->b = 6;

	printf("5 + 6 =\n");

	int err = client->call( "add", argBuffer, resultBuffer );

	if ( err ) {
		fprintf( stderr, "client->call returned error\n" );
	}

	printf(" ---> %d\n", res->c );
}

