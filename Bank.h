
const int NameSize = 80;

class Bank {
public:
	virtual void clear() = 0;
	virtual int numberOfAccounts() = 0;
	virtual int createAccount( char * name, int initialBalance ) = 0;
	virtual int deposit( int accountNumber, int amount ) = 0;
	virtual int transfer( int srcAccountNumber, int destAccountNumber, int amount ) = 0;
	virtual int totalFunds() = 0;
	virtual int accountInfo( int accountNumber, char name[ NameSize ], int * balance ) = 0;
};

