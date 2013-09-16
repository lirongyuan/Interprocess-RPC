#include <stdio.h>
#include <string.h>
#include <thread.h>
#include <synch.h>

#include "RPC.h"
#include "Bank.h"

const int MaxAccounts = 100;

struct Account {
        int _accountNumber;
        char _name[ NameSize ];
        int _balance;
        mutex_t mutex;
};

enum SyncMethod {
	SyncNone = 0,
	SyncGlobalMutex = 1,
	SyncAccountMutex = 2
};
		
class BankServer : public Bank {
	// Number of accounts in this bank
	int _numberOfAccounts;

	// Array of accounts
	Account _accounts[ MaxAccounts ];

	SyncMethod _syncMethod;
	
	mutex_t mutex;
	sema_t sem;

  public:
	BankServer( SyncMethod syncMethod );
	void clear();
	int numberOfAccounts();
	int createAccount( char * name, int initialBalance );
	int deposit( int accountNumber, int amount );
	int transfer( int srcAccountNumber, int destAccountNumber, int amount );
	int totalFunds();
	int accountInfo( int accountNumber, char name [NameSize], 
				int * balance );
};

BankServer::BankServer( SyncMethod syncMethod )
{
	_numberOfAccounts = 0;
	_syncMethod = syncMethod;
	
	mutex_init(&mutex, USYNC_PROCESS, NULL); 
	sema_init(&sem,0,USYNC_PROCESS,NULL);
}

void
BankServer::clear()
{
	_numberOfAccounts = 0;
}

int
BankServer::numberOfAccounts()
{
	return _numberOfAccounts;
}

int
BankServer::createAccount( char * name, int initialBalance )
{
	if ( _numberOfAccounts >= MaxAccounts ) {
		return -1;
	}
	
	if(_syncMethod == SyncGlobalMutex){
		mutex_lock(&mutex);
	}
	
	Account * acc = &_accounts[ _numberOfAccounts ];

	acc->_accountNumber = _numberOfAccounts;
	strncpy( acc->_name, name, NameSize );
	acc->_name[ NameSize - 1 ] = 0;
	acc->_balance = initialBalance;
	_numberOfAccounts++;

	if(_syncMethod == SyncGlobalMutex){
		mutex_unlock(&mutex);
	}
	
	return 0;
}

int 
BankServer::deposit( int accountNumber, int amount )
{
	if ( accountNumber >= _numberOfAccounts ) {
		return -1;
	}
	if(_syncMethod == SyncGlobalMutex){
		mutex_lock(&mutex);
	}
	if(_syncMethod == SyncAccountMutex){
		mutex_lock(&(_accounts[ accountNumber ].mutex));
	}
	_accounts[ accountNumber ]._balance += amount;
	if(_syncMethod == SyncAccountMutex){
		mutex_unlock(&(_accounts[ accountNumber ].mutex));
	}
	if(_syncMethod == SyncGlobalMutex){
		mutex_unlock(&mutex);
	}
	return 0;
}

int 
BankServer::transfer( int srcAccountNumber, int destAccountNumber, int amount )
{
	if ( srcAccountNumber >= _numberOfAccounts ||
	     destAccountNumber >= _numberOfAccounts ||
	     srcAccountNumber == destAccountNumber ||
	     _accounts[ srcAccountNumber ]._balance < amount ) {
		return -1;
	}
	if(_syncMethod == SyncGlobalMutex){
		mutex_lock(&mutex);
	}
	if(_syncMethod == SyncAccountMutex){
		if ( srcAccountNumber< destAccountNumber) {
     		mutex_lock(&(_accounts[ srcAccountNumber ].mutex));
    		mutex_lock(&(_accounts[ destAccountNumber ].mutex));
  		}else{
    		mutex_lock(&(_accounts[ destAccountNumber ].mutex));
    		mutex_lock(&(_accounts[ srcAccountNumber ].mutex));
  		}
	}
	int src = _accounts[ srcAccountNumber ]._balance;
	int dest = _accounts[ destAccountNumber ]._balance;
	src -= amount;
	dest += amount;
	_accounts[ srcAccountNumber ]._balance = src;
	_accounts[ destAccountNumber ]._balance = dest;
	if(_syncMethod == SyncAccountMutex){
    	mutex_unlock(&(_accounts[ destAccountNumber ].mutex));
    	mutex_unlock(&(_accounts[ srcAccountNumber ].mutex));
	}
	if(_syncMethod == SyncGlobalMutex){
		mutex_unlock(&mutex);
	}
	return 0;
}

int 
BankServer::totalFunds()
{
	int total = 0;
	
	if(_syncMethod == SyncGlobalMutex){
		mutex_lock(&mutex);
	}
	if(_syncMethod == SyncAccountMutex){
		for (int i=0;i<_numberOfAccounts; i++){ //lock each account
			mutex_lock(&(_accounts[i].mutex));
		}
	}
	for ( int i = 0; i < _numberOfAccounts; i++ ) {
		total += _accounts[i]._balance;
	}
	if(_syncMethod == SyncAccountMutex){
		for (int i = 0; i<_numberOfAccounts; i++){ //lock each account
			mutex_unlock(&(_accounts[ i ].mutex));
		}
	}
	if(_syncMethod == SyncGlobalMutex){
		mutex_unlock(&mutex);
	}
	return total;
}

int 
BankServer::accountInfo( int accountNumber, char name[ NameSize ], 
				int * balance )
{
	if ( accountNumber >= _numberOfAccounts ) {
		return -1;
	}

	if(_syncMethod == SyncGlobalMutex){
		mutex_lock(&mutex);
	}
	if(_syncMethod == SyncAccountMutex){
		mutex_lock(&(_accounts[ accountNumber ].mutex));
	}
	Account * acc = &_accounts[ accountNumber ];
	memcpy( name, acc->_name, NameSize );
	*balance = acc->_balance;
	if(_syncMethod == SyncAccountMutex){
		mutex_unlock(&(_accounts[ accountNumber ].mutex));
	}
	if(_syncMethod == SyncGlobalMutex){
		mutex_unlock(&mutex);
	}
	return 0;
}

///// Server Stubs /////

class ServerStub {
	static int clear( void * cookie, RPCArgument argument, RPCResult result );
	static int numberOfAccounts( void * cookie, RPCArgument argument, RPCResult result );
	static int createAccount( void * cookie, RPCArgument argument, RPCResult result );
	static int deposit( void * cookie, RPCArgument argument, RPCResult result );
	static int transfer( void * cookie, RPCArgument argument, RPCResult result );
	static int totalFunds( void * cookie, RPCArgument argument, RPCResult result );
	static int accountInfo( void * cookie, RPCArgument argument, RPCResult result );
public:
	static void registerStubs( RPCServer * s, Bank * b );
};

int
ServerStub::clear( void * cookie, RPCArgument argument, RPCResult result )
{
	Bank * bank = (Bank *) cookie;

	bank->clear();
	
	return 0;
}

int
ServerStub::numberOfAccounts( void * cookie, RPCArgument argument, RPCResult result )
{
	Bank * bank = (Bank *) cookie;

	struct Result {
		int a;
	} * res = ( Result * ) result;

	res->a = bank->numberOfAccounts();
	
	return 0;
}

int
ServerStub::createAccount( void * cookie, RPCArgument argument, RPCResult result )
{
	Bank * bank = (Bank *) cookie;

	struct Arguments {
		char a[ NameSize ];
		int b;
	} * arg = ( Arguments * ) argument;
	
	struct Result {
		int a;
	} * res = ( Result * ) result;

	res->a = bank->createAccount( arg->a, arg->b );

	return 0;
}

int
ServerStub::deposit( void * cookie, RPCArgument argument, RPCResult result )
{
	Bank * bank = (Bank *) cookie;

	struct Arguments {
		int a;
		int b;
	} * arg = ( Arguments * ) argument;
	
	struct Result {
		int a;
	} * res = ( Result * ) result;

	res->a = bank->deposit( arg->a, arg->b );

	return 0;
}

int
ServerStub::transfer( void * cookie, RPCArgument argument, RPCResult result )
{
	Bank * bank = (Bank *) cookie;
	
	struct Arguments {
		int a;
		int b;
		int c;
	} * arg = ( Arguments * ) argument;

	struct Result {
		int a;
	} * res = ( Result * ) result;

	res->a = bank->transfer( arg->a, arg->b, arg->c );

	return 0;
}

int
ServerStub::totalFunds( void * cookie, RPCArgument argument, RPCResult result )
{
	Bank * bank = (Bank *) cookie;
	
	struct Result {
		int a;
	} * res = ( Result * ) result;

	res->a = bank->totalFunds();

	return 0;	
}

int
ServerStub::accountInfo( void * cookie, RPCArgument argument, RPCResult result )
{
	Bank * bank = (Bank *) cookie;
	
	struct Arguments {
		int a;
	} * arg = ( Arguments * ) argument;

	struct Result {
		char a[ NameSize ];
		int b;
		int c;
	} * res = ( Result * ) result;

	res->c = bank->accountInfo( arg->a, res->a, &res->b );

	return 0;
}

void
ServerStub::registerStubs( RPCServer * s, Bank * bank )
{
	s->registerRPC( "clear", clear, bank );
	s->registerRPC( "numberOfAccounts", numberOfAccounts, bank );
	s->registerRPC( "createAccount", createAccount, bank );
	s->registerRPC( "deposit", deposit, bank );
	s->registerRPC( "transfer", transfer, bank );
	s->registerRPC( "totalFunds", totalFunds, bank );
	s->registerRPC( "accountInfo", accountInfo, bank );
}

void
printUsage()
{
	fprintf( stderr, 
		"Usage: BankClient [ none | global | account ]\n");
	exit( -1 );
}

int
main( int argc, char ** argv )
{
	if ( argc <= 1 ) {
		printUsage();
	}

	SyncMethod syncMethod;

	if ( !strcmp( argv[ 1 ], "none" ) ) {
		syncMethod = SyncNone;
	}
	else if ( !strcmp( argv[ 1 ], "global" ) ) {
		syncMethod = SyncGlobalMutex;
	}
	else if ( !strcmp( argv[ 1 ], "account" ) ) {
		syncMethod = SyncAccountMutex;
	}	
	else {
		printUsage();
	}

	printf(" Synchronization method is: ");

	if ( syncMethod == SyncNone ) {
		printf( "SyncNone\n" );
	}
	else if ( syncMethod == SyncGlobalMutex ) {
		printf( "SyncGlobalMutex\n" );
	}
	else if ( syncMethod == SyncAccountMutex ) {
		printf( "SyncAccountMutex\n" );
	}

	RPCServer * server = new RPCServer( "bank.rpc", 10 );

	Bank * bank = new BankServer( syncMethod) ;
	
	ServerStub::registerStubs( server, bank );
	
	server->runServer();
}
