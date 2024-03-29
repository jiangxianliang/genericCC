#include <chrono>
#include <fcntl.h>

#include "congctrls.hh"
#include "remycc.hh"
#include "ctcp.hh"
#include "kernelTCP.hh"
//#include "pcc-tcp.hh"
#include "nashcc.hh"
#include "traffic-generator.hh"

// see configs.hh for details
double TRAINING_LINK_RATE = 4000000.0/1500.0;
bool LINK_LOGGING = false;
std::string LINK_LOGGING_FILENAME;

int main( int argc, char *argv[] ){
	Memory temp;
	WhiskerTree whiskers;
	bool ratFound = false;

	string serverip="", sourceip="";
	int serverport=8888, sourceport=0;
	int offduration=5000, onduration=5000;
	double delta=1.0;

	enum CCType { REMYCC, TCPCC, KERNELCC, PCC, NASHCC } cctype = REMYCC;

	for ( int i = 1; i < argc; i++ ) {
		std::string arg( argv[ i ] );
		if ( arg.substr( 0, 3 ) == "if=" ) {
			if( cctype != REMYCC ){
				fprintf( stderr, "Warning: ignoring parameter 'if=' as cctype is not 'remy'.\n" );
				continue;
			}

			std::string filename( arg.substr( 3 ) );
			int fd = open( filename.c_str(), O_RDONLY );
			if ( fd < 0 ) {
				perror( "open" );
				exit( 1 );
			}

			RemyBuffers::WhiskerTree tree;
			if ( !tree.ParseFromFileDescriptor( fd ) ) {
				fprintf( stderr, "Could not parse %s.\n", filename.c_str() );
				exit( 1 );
			}
	
			whiskers = WhiskerTree( tree );
			ratFound = true;

			if ( close( fd ) < 0 ) {
				perror( "close" );
				exit( 1 );
			}
		}
		else if( arg.substr( 0, 9 ) == "serverip=" )
			serverip = arg.substr( 9 );
		else if( arg.substr( 0, 9 ) == "sourceip=" )
			sourceip = arg.substr( 9 );
		else if( arg.substr( 0, 11 ) == "serverport=" )
			serverport = atoi( arg.substr( 11 ).c_str() );
		else if( arg.substr( 0, 11 ) == "sourceport=" )
			sourceport = atoi( arg.substr( 11 ).c_str() );
		else if( arg.substr( 0, 12 ) == "offduration=" )
			offduration	= atoi( arg.substr( 12 ).c_str() );
		else if( arg.substr( 0, 11 ) == "onduration=" )
			onduration = atoi( arg.substr( 11 ).c_str() );
		else if( arg.substr( 0, 9 ) == "linkrate=" ) 
			TRAINING_LINK_RATE = atof( arg.substr( 9 ).c_str() );
		else if( arg.substr( 0, 8 ) == "linklog=" ){
			LINK_LOGGING_FILENAME = arg.substr( 8 );
			LINK_LOGGING = true;
		}
		else if( arg.substr( 0, 6 ) == "delta=" )
			delta = atof( arg.substr( 6 ).c_str() );
		else if( arg.substr( 0, 7 ) == "cctype=" ) {
			std::string cctype_str = arg.substr( 7 );
			if( cctype_str == "remy" )
				cctype = CCType::REMYCC;
			else if( cctype_str == "tcp" )
				cctype = CCType::TCPCC;
			else if ( cctype_str == "kernel" )
				cctype = CCType::KERNELCC;
			else if ( cctype_str == "pcc" )
				cctype = CCType::PCC;
			else if ( cctype_str == "nash" )
				cctype = CCType::NASHCC;
			else
				fprintf( stderr, "Unrecognised congestion control protocol '%s'.\n", cctype_str.c_str() );
		}
		else {
			fprintf( stderr, "Unrecognised option '%s'.\n", arg.c_str() );
		}
	}

	if ( serverip == "" || sourceip == ""){
		fprintf( stderr, "Usage: sender serverip=(ipaddr) sourceip=(ipaddr) [if=(ratname)] [delta=(for NashCC)] [offduration=(time in ms)] [onduration=(time in ms)] [cctype=remy|kernel|tcp|pcc|nash] [linkrate=(packets/sec)] [linklog=filename][serverport=(port)] [sourceport=(port)]\n");
		exit(1);
	}

	if( not ratFound and cctype == CCType::REMYCC ){
		fprintf( stderr, "Please specify remy specification file using if=<filename>\n" );
		exit(1);
	}

	if( cctype == CCType::REMYCC){
		fprintf( stdout, "Using RemyCC.\n" );
		RemyCC congctrl( whiskers );
		CTCP< RemyCC > connection( congctrl, serverip, serverport, sourceip, sourceport );
		TrafficGenerator<CTCP<RemyCC>> traffic_generator( connection, onduration, offduration, TrafficType::EXPONENTIAL_ON_OFF );
		traffic_generator.spawn_senders( 1 );
	}
	else if( cctype == CCType::TCPCC ){
		fprintf( stdout, "Using UDT's TCP CC.\n" );
		DefaultCC congctrl;
		CTCP< DefaultCC > connection( congctrl, serverip, serverport, sourceip, sourceport );
		TrafficGenerator< CTCP< DefaultCC > > traffic_generator( connection, onduration, offduration, TrafficType::EXPONENTIAL_ON_OFF );
		traffic_generator.spawn_senders( 1 );
	}
	else if ( cctype == CCType::KERNELCC ){
		fprintf( stdout, "Using the Kernel's TCP using sockperf.\n");
		KernelTCP connection( serverip, serverport );
		TrafficGenerator< KernelTCP > traffic_generator( connection, onduration, offduration, TrafficType::EXPONENTIAL_ON_OFF );
		traffic_generator.spawn_senders( 1 );
	}
	else if ( cctype == CCType::PCC ){
		fprintf( stdout, "Using PCC.\n");
		assert( false );
		// PCC_TCP connection( serverip, serverport );
		// TrafficGenerator< PCC_TCP > traffic_generator( connection, onduration, offduration, TrafficType::EXPONENTIAL_ON_OFF );
		// traffic_generator.spawn_senders( 1 );
	}
	else if ( cctype == CCType::NASHCC ){
		fprintf( stdout, "Using NashCC\n" );
		NashCC congctrl( delta );
		CTCP< NashCC > connection( congctrl, serverip, serverport, sourceip, sourceport );
		TrafficGenerator<CTCP<NashCC>> traffic_generator( connection, onduration, offduration, TrafficType::EXPONENTIAL_ON_OFF );
		traffic_generator.spawn_senders( 1 );
	}
	else{
		assert( false );
	}
}