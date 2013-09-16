
#include <stdio.h>
#include <thread.h>
#include <assert.h>
#include <stdlib.h>

#include "RPC.h"

int add_in_server( RPCClient * client, int a, int b )
{
	RPCArgument argBuffer;
	RPCResult resultBuffer;

	struct Args {
	  int a;
	  int b;
	} * arg = (Args *) argBuffer;

	struct Res {
		int c;
	} * res = (Res *) resultBuffer;

	arg->a = a;
	arg->b = b;

	int err = client->call( "add", argBuffer, resultBuffer );
	
	if ( err ) {
	  fprintf( stderr, "client->call returned error\n" );
	  exit( 0 );
	}

	return res->c;
}

int add_in_server_with_delay( RPCClient * client, int a, int b, int delay )
{
	RPCArgument argBuffer;
	RPCResult resultBuffer;

	struct Args {
	  int a;
	  int b;
	  int delay;
	} * arg = (Args *) argBuffer;

	struct Res {
		int c;
	} * res = (Res *) resultBuffer;

	arg->a = a;
	arg->b = b;
	arg->delay = delay;

	int err = client->call( "add_with_delay", argBuffer, resultBuffer );
	
	if ( err ) {
	  fprintf( stderr, "client->call returned error\n" );
	  exit( 0 );
	}

	return res->c;
}

void
threadAddWithoutDelay( RPCClient * client )
{
  printf(" threadAddWithoutDelay ready\n");

  int i = 0;
  
  hrtime_t startLoop = gethrtime();

  while ( (gethrtime() - startLoop)/1000000000 < 10 ) {
    // Initial time
    hrtime_t start = gethrtime();
    
    int c = add_in_server( client, 3 * i, i * 5 );
    
    if ( c != 8 * i ) {
      printf("**** Error %d + %d != %d\n",
	     3 * i, i * 5, c );
      exit( -1 );
    }

    // End time
    hrtime_t end = gethrtime();
    
    i++;
    if ( end - start > 500000000 ) {
      printf("Call %d took %5.3lf secs. There is something wrong\n",
	     i, (end - start)/1000000000.0 );
      exit( -1 );
    }

    if ( i % 10000 == 0 ) {
      printf("%d calls made\n", i);
    }
  }
}

void
threadAddWithDelay( RPCClient * client )
{
  printf(" threadAddWithDelay ready\n");

  for ( int i = 0; i < 10; i++ ) {
    int c = add_in_server_with_delay( client, 3 * i, i * 5, 1 );
    if ( c != 8 * i ) {
      printf("**** Error %d + %d != %d\n",
	     3 * i, i * 5, c );
      exit( -1 );
    }
  }
}

int
main( int argv, char ** argc )
{
  RPCClient * client = new RPCClient( "server2.rpc" );
  assert( client != 0 );

  thread_t t1;
  thr_create( 0, 0, (void *(*)(void *)) threadAddWithoutDelay, 
	      (void *) client, THR_BOUND, &t1 );
  
  thread_t t2;
  thr_create( 0, 0, (void *(*)(void *)) threadAddWithDelay, 
	      (void *) client, THR_BOUND, &t2 );

  printf("Waiting for threads\n");
  
  thr_join( t1, 0, 0 );
  printf("threadAddWithoutDelay returned\n");

  thr_join( t2, 0, 0 );
  printf("threadAddWithDelay returned\n");
  
  printf(">>>> Test passed!!\n");
  return 0;
}

