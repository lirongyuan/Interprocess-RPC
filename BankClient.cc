
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <thread.h>

extern "C" int rand_r(unsigned int *seed);

#include "RPC.h"
#include "Bank.h"


class BankClient : public Bank {
	RPCClient * _rpcClient;
public:
	BankClient( RPCClient * rpcClient );
	int numberOfAccounts();
	int createAccount( char * name, int initialBalance );
	int deposit( int accountNumber, int amount );
	int transfer( int srcAccountNumber, int destAccountNumber, int amount );
	int totalFunds();
	int accountInfo( int accountNumber, char name[ NameSize ], 
				int * balance );
	void clear();
};

void
PrintErrorAndExit( char * msg, int err )
{
	fprintf( stderr, "%s err=%d\n", msg, err );
	exit( -1 );
}

BankClient::BankClient( RPCClient * rpcClient )
{
	_rpcClient = rpcClient;
}

void
BankClient::clear()
{
	RPCArgument argument;
	RPCResult result;
	
	int err = _rpcClient->call( "clear", argument, result );

	if ( err ) {
		PrintErrorAndExit( "clear:", err );
	}
}

int
BankClient::numberOfAccounts()
{
	RPCArgument argument;
	RPCResult result;
	
	struct Result {
		int a;
	} * res = ( Result * ) result;

	int err = _rpcClient->call( "numberOfAccounts", argument, result );

	if ( err ) {
		PrintErrorAndExit( "numberOfAccounts:", err );
	}

	return res->a;
}

int
BankClient::createAccount( char * name, int initialBalance )
{
	RPCArgument argument;
	RPCResult result;
	
	struct Arguments {
		char a[ NameSize ];
		int b;
	} * arg = ( Arguments * ) argument;
	
	struct Result {
		int a;
	} * res = ( Result * ) result;

	strncpy( arg->a, name, NameSize );
	arg->a[ NameSize - 1 ] = 0;
	arg->b = initialBalance;

	int err = _rpcClient->call( "createAccount", argument, result );

	if ( err ) {
		PrintErrorAndExit( "createAccount:", err );
	}

	return res->a;
}

int 
BankClient::deposit( int accountNumber, int amount )
{
	RPCArgument argument;
	RPCResult result;

	struct Arguments {
		int a;
		int b;
	} * arg = ( Arguments * ) argument;
	
	struct Result {
		int a;
	} * res = ( Result * ) result;

	arg->a = accountNumber;
	arg->b = amount;

	int err = _rpcClient->call( "deposit", argument, result );

	if ( err ) {
		PrintErrorAndExit( "deposit:", err );
	}

	return res->a;
}

int 
BankClient::transfer( int srcAccountNumber, int destAccountNumber, int amount )
{
	RPCArgument argument;
	RPCResult result;

	struct Arguments {
		int a;
		int b;
		int c;
	} * arg = ( Arguments * ) argument;
	
	struct Result {
		int a;
	} * res = ( Result * ) result;

	arg->a = srcAccountNumber;
	arg->b = destAccountNumber;
	arg->c = amount;

	int err = _rpcClient->call( "transfer", argument, result );

	if ( err ) {
		PrintErrorAndExit( "transfer:", err );
	}

	return res->a;
}

int 
BankClient::totalFunds()
{
	RPCArgument argument;
	RPCResult result;

	struct Result {
		int a;
	} * res = ( Result * ) result;

	int err = _rpcClient->call( "totalFunds", argument, result );

	if ( err ) {
		PrintErrorAndExit( "totalFunds:", err );
	}

	return res->a;
}

int 
BankClient::accountInfo( int accountNumber, char name[ NameSize ], 
				int * balance )
{
	RPCArgument argument;
	RPCResult result;

	struct Arguments {
		int a;
	} * arg = ( Arguments * ) argument;
	
	struct Result {
		char a[ NameSize ];
		int b;
		int c;
	} * res = ( Result * ) result;

	arg->a = accountNumber;
	int err = _rpcClient->call( "accountInfo", argument, result );

	if ( err ) {
		PrintErrorAndExit( "accountInfo:", err );
	}

	memcpy( name, res->a, NameSize );
	* balance = res->b; 

	return res->c;
}

void
test1( Bank * bank)
{
	printf("\n------- Test 1 ----------\n");

	bank->clear();

	printf( "createAccount( \"George\", 1000 )\n" );
	int err = bank->createAccount( "George", 1000 );
	if ( err ) {
		fprintf( stderr, "Could not createAccount\n" );
	}

	char name[ NameSize ];
	int b;
	err = bank->accountInfo( 0, name, &b );
	if ( err ) {
		fprintf( stderr, "Could not call accountInfo" );
	}

	printf("accountInfo(0,%s,%d)\n", name, b );
	if ( !strcmp("George", name ) && b == 1000 ) {
		printf("--> test1 seems to be OK\n\n");
	} 
	else {
		printf("--> test1 failed\n\n");
	}
}

void
printAccounts( Bank * bank )
{
	int numberOfAccounts = bank->numberOfAccounts();

	printf("Name       Balance\n");
	printf("---------- -------\n");
	for ( int i = 0; i < numberOfAccounts; i++ ) {
		char name[ NameSize ];
		int balance;
		bank->accountInfo( i, name, &balance );
		printf("%-10s %7d\n", name, balance );
	}
	printf("---------- -------\n");

	int total = bank->totalFunds();

	printf("Total:     %7d\n", total );
}

void
createAccounts( Bank * bank )
{
	bank->createAccount( "George", 1000 );
	bank->createAccount( "Peter", 1000 );
	bank->createAccount( "John", 1000 );
	bank->createAccount( "Jerry", 1000 ); 
	bank->createAccount( "Kramer", 1000 );
	bank->createAccount( "Elaine", 1000 );
	bank->createAccount( "Charles", 1000 );
	bank->createAccount( "Mike", 1000 );
	bank->createAccount( "Debra", 1000 );
	bank->createAccount( "Maude", 1000 );
}

int testWrong;
int testNumberOfTransfers;

void
testTransfersThread( Bank * bank )
{
	printf("testTransfersThread ready\n");

	unsigned seed = (unsigned) &bank;

	int numberOfAccounts = bank->numberOfAccounts();
	for ( int i = 0; i < testNumberOfTransfers; i++ ) {
		int from = rand_r( &seed ) % numberOfAccounts;; 
		int to = rand_r( &seed ) % numberOfAccounts;
		bank->transfer( from, to, 1 );

		if ( (i % 1000) == 0 ) {
			int total = bank->totalFunds();
	
			int ok = (total == 10 * 1000);

			printf("Thread %d, Transaction: %d of %d, "
				"TotalFunds: %d %s\n", 
				thr_self(), i, testNumberOfTransfers, total,
				 ok?"OK":"Wrong!!");	

			if ( !ok ) {
				testWrong = 1;
			}
		}
	}

	printf("Return testTransfersThread\n");
}

void
testTransfers( Bank * bank, int numberOfTransfers, int numberOfThreads, 
		int testNumber  ) 
{
	printf("\n------- Test %d ----------\n", testNumber );
	printf("numberOfTransfers=%d numberOfThreads=%d\n",
		numberOfTransfers, numberOfThreads );

	testNumberOfTransfers = numberOfTransfers;

	bank->clear();

	createAccounts( bank );
	printAccounts( bank );

	thread_t * t = new thread_t[ numberOfThreads ];
	for ( int i = 0; i < numberOfThreads; i++ ) {
		thr_create( 0, 0, (void *(*)(void *)) testTransfersThread, 
				(void *) bank, THR_BOUND, &t[ i ] );
	}

	for (int i = 0; i < numberOfThreads; i++ ) {
		printf("Wait for %d\n", t[ i ]);
		thr_join( t[ i ], 0, 0 );
	}

	int total = bank->totalFunds();

	printAccounts( bank );

	if ( !testWrong ) {
		printf("--> test%d seems to be OK\n\n",
			testNumber );
	} 
	else {
		printf("--> test%d failed\n\n",
			testNumber );
	}
}

void
printUsage()
{
	printf( "Usage: BankClient [ 1 | 2 | 3 | 4 ]\n");
	exit( 0 );
}

int
main( int argv, char ** argc )
{
	RPCClient * rpcClient = new RPCClient( "bank.rpc" );
	Bank * bank = new BankClient( rpcClient );

	if ( argv < 2 ) {
		printUsage();
	}

	hrtime_t start = gethrtime();

	if ( !strcmp( argc[ 1 ], "1" ) ) {
		test1( bank );
	}
	else if ( !strcmp( argc[ 1 ], "2" ) ) {
		testTransfers( bank, 25000, 1, 2 );
	}
	else if ( !strcmp( argc[ 1 ], "3" ) ) {
		testTransfers( bank, 25000, 2, 3 );
	}
	else if ( !strcmp( argc[ 1 ], "4" ) ) {
		testTransfers( bank, 25000, 10, 4 );
	}
	else {
		printUsage();
	}

	hrtime_t end = gethrtime();

	printf(" Total time:             %10.3lfs\n", 
		(end - start)/1000000000.0 );

	printf(" Max simultaneous calls: %10d\n",
		rpcClient->maxSimultaneousCalls() );

	printf(" Total calls:            %10d\n",
		rpcClient->totalCalls() );
}

