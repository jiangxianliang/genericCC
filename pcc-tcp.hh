#ifndef PCC_TCP_HH
#define PCC_TCP_HH

#include <assert.h>
#include <chrono>

class PCC_TCP{
private:
	string dstaddr;
	int dstport;

	double current_timestamp( chrono::high_resolution_clock::time_point &start_time_point );

public:

	PCC_TCP( string ipaddr, int port ) 
	: 	dstaddr( ipaddr ),
		dstport( port )
	{ }

	//duration in milliseconds
	void send_data ( double duration, int flow_id, int src_id );

	void listen_for_data ( ) { assert( false ); };
};

#endif