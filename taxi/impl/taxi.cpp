//--------------------------------------------
// TAXI PROGRAMMING LANGUAGE
// by: BigZaphod sean@fifthace.com
// http://www.bigzaphod.org/taxi/
//
// License: Public Domain
//--------------------------------------------
// Version 1.5
//--------------------------------------------


#include <stdarg.h>
#include <stdlib.h>
#include <cmath>
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <list>
#include <vector>
#include <sstream>
#include <algorithm>
#include <set>
using namespace std;

#ifdef WIN32
	#define random rand
	#define srandom srand
#endif

void error( const char* msg )
{
	cout.flush();
	cerr << "error: " << msg << endl;
	exit( -1 );
}

template <class T>
string to_string( const T& val )
{
        ostringstream buffer;
        buffer << val;
        return buffer.str();
}

string to_lower( string s )
{
        transform( s.begin(), s.end(), s.begin(), (int(*)(int))tolower );
        return s;
}

string to_upper( string s )
{
        transform( s.begin(), s.end(), s.begin(), (int(*)(int))toupper );
        return s;
}

string trim( const string& s )
{
        string r = s;
        r.erase( 0, r.find_first_not_of(" \r\n\t") );
        r = r.substr( 0, 1+r.find_last_not_of(" \r\n\t") );
        return r;
}

typedef enum { D_WEST, D_NORTH, D_SOUTH, D_EAST } nesw_t;
typedef enum { D_LEFT, D_RIGHT } rel_dir_t;

class taxi_value
{
	typedef enum { T_NUM, T_STR } value_type;

	value_type	val_type;
	double		num_val;
	string		str_val;

public:
	taxi_value()				{ clear(); }
	taxi_value( double n ) 			{ set_num(n); }
	taxi_value( const string& s ) 		{ set_str(s); }
	taxi_value( const taxi_value& v )	{ *this = v; }

	taxi_value& operator=( const taxi_value& v ) {
		val_type = v.val_type;
		num_val = v.num_val;
		str_val = v.str_val;
		return *this;
	}

	void clear()			{ val_type = T_NUM; num_val = 0; str_val.clear(); }
	void set_num( double n )	{ val_type = T_NUM; num_val = n; }
	void set_str( const string& s )	{ val_type = T_STR; str_val = s; }

	bool is_number() const 		{ return val_type == T_NUM; }
	bool is_string() const 		{ return val_type == T_STR; }

	double num() const		{ return num_val; }
	string str() const		{ return str_val; }
};

typedef list<taxi_value> value_list;

class location;
class taxi;
class node;
class location;

class passenger_t
{
public:
	passenger_t() : dest(NULL), distance_traveled(0) {}
	passenger_t( const taxi_value& v, location* d ) : dest(d), distance_traveled(0), value(v) {}
	passenger_t( const passenger_t& p ) { *this = p; }
	passenger_t& operator=( const passenger_t& p ) {
		dest = p.dest;
		value = p.value;
		distance_traveled = p.distance_traveled;
		return *this;
	}

	location* dest;
	double distance_traveled;
	taxi_value value;
};
typedef list<passenger_t> passenger_list;


class taxi 
{
public:
	passenger_list passengers;
	nesw_t direction;
	node* current_node;
	location* current_location;
	node* next_node;
	node* prev_node;
	const double pixels_per_mile;
	const double max_gas;
	double gas;
	double gas_usage;
	double credits;
	double fare;
	double miles_driven;

	taxi( location* start_from, double ppm, double max_gas_, double gas_, double gas_usage_, double credits_, double fare_ );
	void pickup_passenger( location* whereto );
	void passenger_pays( passenger_t& p );

	void set_direction( nesw_t dir );
	void take_xth_turn( int num, rel_dir_t dir );
	void drive_to( location* to );

private:
	void drive();
	void turn( rel_dir_t dir ) {
		if( direction == D_WEST ) direction=(dir==D_LEFT)? 	D_SOUTH: D_NORTH;
		else if( direction == D_EAST ) direction=(dir==D_LEFT)?	D_NORTH: D_SOUTH;
		else if( direction == D_NORTH ) direction=(dir==D_LEFT)?D_WEST: D_EAST;
		else if( direction == D_SOUTH) direction=(dir==D_LEFT)?	D_EAST: D_WEST;
	}
};

class incoming_list
{
	taxi& car;
	passenger_list incoming;

public:
	incoming_list( taxi& c, location* dest ) : car(c) {
		for( passenger_list::iterator i=car.passengers.begin(); i!=car.passengers.end(); ) {
			if( i->dest == dest ) {
				incoming.push_back(*i);
				i = car.passengers.erase( i );
			} else {
				i++;
			}
		}
	}

	int size() const	{ return incoming.size(); }
	bool empty() const 	{ return incoming.empty(); }

	taxi_value next() {
		if( empty() )
			error( "cannot read incoming list" );

		taxi_value r = incoming.front().value;
		car.passenger_pays( incoming.front() );
		incoming.pop_front();
		return r;
	}

	void update_taxi() 	{ car.passengers.splice( car.passengers.end(), incoming ); incoming.clear(); }
};

class node
{
public:
	int x;
	int y;

	typedef list< pair<node*,node*> > neighbor_list;
	neighbor_list neighbors;

	node( int x_=0, int y_=0 ) { position( x_, y_ ); }
	void position( int x_, int y_ ) { x=x_; y=y_; }

	static void street( int num_nodes, ... ) {
		if( num_nodes < 2 )
			return;
		node* p = NULL;
		va_list args;
		va_start( args, num_nodes );
		while( num_nodes-- ) {
			pair<node*,node*> e;
			node* n = va_arg( args, node* );
			if( p ) {
				p->neighbors.back().second = n;
				e.first = p;
				e.second = NULL;
			} else {
				e.first = NULL;
				e.second = NULL;
			}
			n->neighbors.push_back( e );
			p = n;
		}
		va_end( args );
	}

	bool is_intersection() const { return neighbors.size() > 1; }

	node* get_straight_path( node* from ) {
		for( neighbor_list::iterator i=neighbors.begin(); i!=neighbors.end(); i++ ) {
			if( i->first == from ) return i->second;
			else if( i->second == from ) return i->first;
		}
		return NULL;
	}

	node* get( nesw_t dir, node* prev=NULL ) {
		for( neighbor_list::iterator i=neighbors.begin(); i!=neighbors.end(); i++ ) {
			if( !prev || (i->first != prev && i->second != prev) ) {
				for( int z=0; z<2; z++ ) {
					node* n = (z==0)? i->first: i->second;
					if( !n ) continue;
					bool b = false;
					switch( dir ) {
					case D_NORTH:	b = ( n->y < y );	break;
					case D_SOUTH:	b = ( n->y > y );	break;
					case D_EAST:	b = ( n->x > x );	break;
					case D_WEST:	b = ( n->x < x );	break;
					}
					if( b ) return n;
				}
			}
		}

		return NULL;
	}

	double dist_to( node* n ) {
		double a = fabs( (double)(x - n->x) );
		double b = fabs( (double)(y - n->y) );
		a *= a;
		b *= b;
		return sqrt( a + b );
	}

	node* can_turn( rel_dir_t dir, node* prev ) {
		for( neighbor_list::iterator i=neighbors.begin(); i!=neighbors.end(); i++ ) {
			if( i->first != prev && i->second != prev ) {
				for( int z=0; z<2; z++ ) {
					node* n = (z==0)? i->first: i->second;
					if( !n ) continue;
					int r = ((x - prev->x)*(n->y - prev->y)) + ((y - prev->y)*(prev->x - n->x));
					switch( dir ) {
					case D_LEFT:	if( r < 0 ) return n;	break;
					case D_RIGHT:	if( r > 0 ) return n;	break;
					}
				}
			}
		}
		return NULL;
	}
};


typedef enum { B_FIFO, B_LIFO, B_RANDOM } buffer_type;
typedef void (*arrival_func)( location&, incoming_list& );
typedef void (*create_outgoing_passenger_func)( location& );
typedef void (*create_waiting_passenger_func)( location&, const char* );

class location : public node
{
	value_list outgoing;

public:
	location() : buffer_order(B_FIFO)
		, max_passengers(-1)	// infinite
		, arrival_function(NULL)
		, create_outgoing_passenger_function(NULL)
		, create_waiting_passenger_function(NULL)
		, passengers_pay(true)
		, gas_price(0) {}

	buffer_type buffer_order;
	int max_passengers;
	arrival_func arrival_function;
	create_outgoing_passenger_func create_outgoing_passenger_function;
	create_waiting_passenger_func create_waiting_passenger_function;
	bool passengers_pay;
	double gas_price;

	void add_outgoing_passenger( const taxi_value& v ) {
		if( max_passengers == 0 || (int)outgoing.size() == max_passengers )
			error( "too many outgoing passengers" );

		switch( buffer_order ) {
		case B_FIFO:	outgoing.push_back(v); break;
		case B_LIFO: 	outgoing.push_front(v); break;
		case B_RANDOM:
			{
				value_list::iterator i=outgoing.begin();
				size_t ran = size_t((1+outgoing.size()) * (random() / (RAND_MAX+1.0)));
				for( ; ran; ran--, i++ );
				outgoing.insert( i, v );
			}
		}
	}

	void arrival( incoming_list& incoming )	{
//		cout  << "outgoing:";
//		for( value_list::iterator i=outgoing.begin(); i!=outgoing.end(); i++ ) {
//			cout << " [";
//			if( i->is_number() )
//				cout << i->num();
//			else
//				cout << i->str();
//			cout << "]";
//		}
//		cout << endl;


		if( arrival_function ) {
			(*arrival_function)( *this, incoming );
		} else {
			while( !incoming.empty() && (max_passengers == -1 || (int)outgoing.size()<max_passengers) ) {
				add_outgoing_passenger( incoming.next() );
			}
		}
	}

	int outgoing_passengers() const { return outgoing.size(); }
	taxi_value get_outgoing_passenger() {
		if( max_passengers == 0 )
			error( "no outgoing passengers allowed at this location" );

		if( outgoing_passengers() == 0 && create_outgoing_passenger_function )
			(*create_outgoing_passenger_function)( *this );

		if( outgoing_passengers() == 0 )
			error( "no outgoing passengers found" );

		taxi_value r = outgoing.front();
		outgoing.pop_front();
		return r;
	}

	void waiting( const char* input ) {
		if( max_passengers == 0 )
			error( "no outgoing passengers allowed at this location" );
		if( !create_waiting_passenger_function )
			error( "passengers cannot be made to wait here" );
		if( max_passengers != -1 && outgoing_passengers() == max_passengers )
			error( "too many passengers already waiting here" );
		(*create_waiting_passenger_function)( *this, input );
	}
};

taxi::taxi( location* start_from, double ppm, double max_gas_, double gas_, double gas_usage_, double credits_, double fare_ )
	: direction(D_NORTH), current_node(dynamic_cast<node*>(start_from))
	, current_location(start_from), next_node(NULL), prev_node(NULL)
	, pixels_per_mile(ppm)
	, max_gas(max_gas_), gas(gas_), gas_usage(gas_usage_), credits(credits_), fare(fare_)
	, miles_driven(0)
{
}

void taxi::pickup_passenger( location* whereto )
{
	if( !current_location )
		error( "not at a location where passengers can even be picked up from" );

	if( passengers.size() == 3 )
		error( "too many passengers in the taxi" );

	passengers.push_back( passenger_t(current_location->get_outgoing_passenger(),whereto) );
}

void taxi::drive() {
	if( !next_node )
		error( "cannot drive in that direction" );
	node* t = next_node->get_straight_path( current_node );
	prev_node = current_node;
	current_node = next_node;
	next_node = t;
	current_location = NULL;

	// accounting...
	double dist = prev_node->dist_to( current_node ) / pixels_per_mile;
	miles_driven += dist;
	gas -= (1/gas_usage) * dist;
	if( gas < 0 )
		error( "out of gas" );

	for( list<passenger_t>::iterator i=passengers.begin(); i!=passengers.end(); i++ )
		i->distance_traveled += dist;
}

void taxi::set_direction( nesw_t dir )
{
	direction = dir;
	next_node = current_node->get( direction );
}

void taxi::take_xth_turn( int num, rel_dir_t dir )
{
	node* t = NULL;
	for( ; num>0; num-- ) {
		drive();
		while( !(t=current_node->can_turn(dir,prev_node)) )
			drive();
	}
	next_node = t;
	turn( dir );
}

void taxi::drive_to( location* to )
{
	node* n = dynamic_cast<node*>(to);
	while( current_node != n )
		drive();
	current_location = to;
	incoming_list il( *this, current_location );
	current_location->arrival( il );
	il.update_taxi();
	if( current_location->gas_price ) {
		double gallons = min( max_gas-gas, credits/current_location->gas_price );
		gas += gallons;
		credits -= gallons * current_location->gas_price;
	}
}

void taxi::passenger_pays( passenger_t& p )
{
	if( p.dest->passengers_pay ) {
		credits += p.distance_traveled * fare;
	}
}

void taxi_garage( location& here, incoming_list& incoming )
{
	cout << endl << "The taxi is back in the garage.  Program complete." << endl;
	exit( 0 );
}

void post_office_arrive( location& here, incoming_list& incoming )
{
	while( !incoming.empty() ) {
		taxi_value v = incoming.next();
		if( !v.is_string() )
			error( "post office cannot handle non-string values" );
		cout << v.str();
	}
}

void post_office_create( location& here )
{
	string line;
	getline( cin, line );
	here.add_outgoing_passenger( taxi_value(line) );
}

void heisenbergs( location& here )
{
	double r = random();
	here.add_outgoing_passenger( taxi_value(r) );
}

void starchild_numerology( location& here, const char* input )
{
	here.add_outgoing_passenger( taxi_value(strtod(input,NULL)) );
}

void writers_depot( location& here, const char* input )
{
	here.add_outgoing_passenger( taxi_value(input) );
}

void the_babelfishery( location& here, incoming_list& incoming )
{
	while( !incoming.empty() ) {
		taxi_value v = incoming.next();
		if( v.is_number() ) {
			here.add_outgoing_passenger( taxi_value(to_string(v.num())) );
		} else if( v.is_string() ) {
			here.add_outgoing_passenger( taxi_value(strtod(v.str().c_str(),NULL)) );
		} else error( "unknown type cannot be translated" );
	}
}

void charboil_grill( location& here, incoming_list& incoming )
{
	while( !incoming.empty() ) {
		taxi_value v = incoming.next();
		if( v.is_string() && v.str().length() > 1 )
			error( "charboil grill can only handle strings of length 1" );
		if( v.is_string() )
			here.add_outgoing_passenger( taxi_value((double)v.str()[0]) );
		else if( v.is_number() )
			here.add_outgoing_passenger( taxi_value( string(1,(char)v.num()) ) );
		else
			error( "unknown data type" );
	}
}

void addition_alley( location& here, incoming_list& incoming )
{
	if( !incoming.empty() ) {
		double ret = 0;

		while( !incoming.empty() ) {
			taxi_value v = incoming.next();
			if( !v.is_number() )
				error( "requires a numerical value" );
			ret += v.num();
		}

		here.add_outgoing_passenger( taxi_value(ret) );
	}
}

void multiplication_station( location& here, incoming_list& incoming )
{
	if( !incoming.empty() ) {
		double ret = 1;

		while( !incoming.empty() ) {
			taxi_value v = incoming.next();
			if( !v.is_number() )
				error( "requires a numerical value" );
			ret *= v.num();
		}

		here.add_outgoing_passenger( taxi_value(ret) );
	}
}

void divide_and_conquer( location& here, incoming_list& incoming )
{
	if( !incoming.empty() ) {
		taxi_value v = incoming.next();
		if( !v.is_number() )
			error( "requires a numerical value" );
		double ret = v.num();

		while( !incoming.empty() ) {
			v = incoming.next();
			if( !v.is_number() )
				error( "requires a numerical value" );
			if( v.num() == 0 )
				error( "divide by zero" );
			ret /= v.num();
		}

		here.add_outgoing_passenger( taxi_value(ret) );
	}
}

void whats_the_difference( location& here, incoming_list& incoming )
{
	if( !incoming.empty() ) {
		taxi_value v = incoming.next();
		if( !v.is_number() )
			error( "requires a numerical value" );
		double ret = v.num();

		while( !incoming.empty() ) {
			v = incoming.next();
			if( !v.is_number() )
				error( "requires a numerical value" );
			ret -= v.num();
		}

		here.add_outgoing_passenger( taxi_value(ret) );
	}
}

void konkats( location& here, incoming_list& incoming )
{
	if( !incoming.empty() ) {
		string ret;

		while( !incoming.empty() ) {
			taxi_value v = incoming.next();
			if( !v.is_string() )
				error( "requires a string value" );
			ret.append( v.str() );
		}

		here.add_outgoing_passenger( taxi_value(ret) );
	}
}

void magic_eight( location& here, incoming_list& incoming )
{
	if( incoming.size() < 2 )
		error( "requires two passengers" );

	taxi_value v1 = incoming.next();
	taxi_value v2 = incoming.next();

	if( !(v1.is_number() && v2.is_number()) )
		error( "requires numerical values" );

	if( v1.num() < v2.num() )
		here.add_outgoing_passenger( v1 );
}

void collator_express( location& here, incoming_list& incoming )
{
	if( incoming.size() < 2 )
		error( "requires two passengers" );

	taxi_value v1 = incoming.next();
	taxi_value v2 = incoming.next();

	if( !(v1.is_string() && v2.is_string()) )
		error( "requires numerical values" );

	if( v1.str() < v2.str() )
		here.add_outgoing_passenger( v1 );
}

void the_underground( location& here, incoming_list& incoming )
{
	if( !incoming.empty() ) {
		taxi_value v1 = incoming.next();

		if( !(v1.is_number()) )
			error( "requires a numerical value" );

		double val = v1.num() - 1;
		if( val > 0 )
			here.add_outgoing_passenger( taxi_value(val) );
	}
}

void riverview_bridge( location& here, incoming_list& incoming )
{
	// this trashes values from existence.  sleep with the fishies!
	while( !incoming.empty() )
		incoming.next();
}

void auctioneer_school( location& here, incoming_list& incoming )
{
	while( !incoming.empty() ) {
		taxi_value v = incoming.next();
		if( !v.is_string() )
			error( "requires a string value" );
		here.add_outgoing_passenger( taxi_value(to_upper(v.str())) );
	}
}

void little_league_field( location& here, incoming_list& incoming )
{
	while( !incoming.empty() ) {
		taxi_value v = incoming.next();
		if( !v.is_string() )
			error( "requires a string value" );
		here.add_outgoing_passenger( taxi_value(to_lower(v.str())) );
	}
}

void toms_trims( location& here, incoming_list& incoming )
{
	while( !incoming.empty() ) {
		taxi_value v = incoming.next();
		if( !v.is_string() )
			error( "requires a string value" );
		here.add_outgoing_passenger( taxi_value(trim(v.str())) );
	}
}

void trunkers( location& here, incoming_list& incoming )
{
	while( !incoming.empty() ) {
		taxi_value v = incoming.next();
		if( !v.is_number() )
			error( "requires a numerical value" );
		here.add_outgoing_passenger( taxi_value((int)v.num()) );
	}
}

void rounders_pub( location& here, incoming_list& incoming )
{
	while( !incoming.empty() ) {
		taxi_value v = incoming.next();
		if( !v.is_number() )
			error( "requires a numerical value" );
		here.add_outgoing_passenger( taxi_value(round(v.num())) );
	}
}

void knots_landing( location& here, incoming_list& incoming )
{
	while( !incoming.empty() ) {
		taxi_value v = incoming.next();
		if( !v.is_number() )
			error( "requires a numerical value" );
		here.add_outgoing_passenger( taxi_value( (double)(!v.num()) ) );
	}
}

void cyclone( location& here, incoming_list& incoming )
{
	while( !incoming.empty() ) {
		taxi_value v = incoming.next();
		here.add_outgoing_passenger( v );
		here.add_outgoing_passenger( v );
	}
}

void chop_suey( location& here, incoming_list& incoming )
{
	while( !incoming.empty() ) {
		taxi_value v = incoming.next();
		if( !v.is_string() ) 
			error( "requires a string value" );
		for( size_t i=0; i<v.str().length(); i++ )
			here.add_outgoing_passenger( taxi_value( string(1,v.str()[i]) ) );
	}
}

void crime_lab( location& here, incoming_list& incoming )
{
	if( incoming.size() < 2 ) {
		error( "requires at least 2 passengers" );
		return;
	}

	taxi_value v = incoming.next();
	if( !v.is_string() ) {
		error( "requires a string value" );
		return;
	}

	do {
		taxi_value tmp = incoming.next();
		if( !tmp.is_string() ) {
			error( "requires a string value" );
			return;
		}
		if( v.str() != tmp.str() )
			return;
	} while( !incoming.empty() );

	here.add_outgoing_passenger( v );
}

void equals_corner( location& here, incoming_list& incoming )
{
	if( incoming.size() < 2 ) {
		error( "requires at least 2 passengers" );
		return;
	}
	taxi_value v = incoming.next();
	if( !v.is_number() ) {
		error( "requires a numerical value" );
		return;
	}

	do {
		taxi_value tmp = incoming.next();
		if( !tmp.is_number() ) {
			error( "requires a numerical value" );
			return;
		}
		if( v.num() != tmp.num() )
			return;
	} while( !incoming.empty() );

	here.add_outgoing_passenger( v );
}

class road_map
{
	map<string,location> dest;
	map<unsigned int, node*> inter;
	map<unsigned int, node*> corner;

public:
	road_map() {
		// setup the intersections
		inter[1] = new node( 424, 145 );
		inter[2] = new node( 596, 138 );
		inter[3] = new node( 1120, 115 );
		inter[4] = new node( 1285, 112 );
		inter[5] = new node( 1370, 108 );
		inter[6] = new node( 1295, 84 );
		inter[7] = new node( 1094, 78 );
		inter[8] = new node( 355, 222 );
		inter[9] = new node( 215, 376 );
		inter[10]= new node( 482, 468 );
		inter[11]= new node( 379, 638 );
		inter[12]= new node( 246, 529 );
		inter[13]= new node( 291, 783 );
		inter[14]= new node( 209, 916 );
		inter[15]= new node( 501, 910 );
		inter[16]= new node( 50, 639 );
		inter[17]= new node( 739, 557 );
		inter[18]= new node( 702, 374 );
		inter[19]= new node( 875, 740 );
		inter[20]= new node( 991, 599 );
		inter[21]= new node( 1003, 825 );
		inter[22]= new node( 1241, 963 );
		inter[23]= new node( 1155, 709 );
		inter[24]= new node( 1382, 716 );
		inter[25]= new node( 1118, 683 );
		inter[26]= new node( 1132, 634 );
		inter[27]= new node( 1437, 617 );
		inter[28]= new node( 1171, 503 );
		inter[29]= new node( 1061, 407 );
		inter[30]= new node( 1061, 445 );
		inter[31]= new node( 1197, 414 );
		inter[32]= new node( 1227, 313 );
		inter[33]= new node( 1372, 314 );
		inter[34]= new node( 1160, 920 );

		// setup the corners
		corner[1] = new node( 510, 52 );
		corner[2] = new node( 108, 492 );
		corner[3] = new node( 682, 442 );
		corner[4] = new node( 818, 710 );
		corner[5] = new node( 1106, 724 );
		corner[6] = new node( 1181, 847 );

		// configure destinations
		dest["Taxi Garage"].arrival_function = &taxi_garage;
		dest["Taxi Garage"].position( 1246, 639 );

		dest["Post Office"].arrival_function = &post_office_arrive;
		dest["Post Office"].create_outgoing_passenger_function = &post_office_create;
		dest["Post Office"].position( 910, 695 );

		dest["Heisenberg\'s"].create_outgoing_passenger_function = &heisenbergs;
		dest["Heisenberg\'s"].position( 1372, 237 );

		dest["Starchild Numerology"].create_waiting_passenger_function = &starchild_numerology;
		dest["Starchild Numerology"].position( 278, 917 );

		dest["Writer\'s Depot"].create_waiting_passenger_function = &writers_depot;
		dest["Writer\'s Depot"].position( 164, 433 );;

		dest["The Babelfishery"].arrival_function = &the_babelfishery;
		dest["The Babelfishery"].position( 949, 879 );

		dest["Charboil Grill"].arrival_function = &charboil_grill;
		dest["Charboil Grill"].position( 152, 702 );

		dest["Addition Alley"].arrival_function = &addition_alley;
		dest["Addition Alley"].position( 652, 211 );

		dest["Multiplication Station"].arrival_function = &multiplication_station;
		dest["Multiplication Station"].position( 1286, 888 );

		dest["Divide and Conquer"].arrival_function = &divide_and_conquer;
		dest["Divide and Conquer"].position( 1117, 311 );

		dest["What\'s The Difference"].arrival_function = &whats_the_difference;
		dest["What\'s The Difference"].position( 176, 153 );;

		dest["KonKat\'s"].arrival_function = &konkats;
		dest["KonKat\'s"].position( 1262, 195 );

		dest["Magic Eight"].arrival_function = &magic_eight;
		dest["Magic Eight"].position( 797, 666 );

		dest["Riverview Bridge"].arrival_function = &riverview_bridge;
		dest["Riverview Bridge"].passengers_pay = false;
		dest["Riverview Bridge"].position( 888, 127 );

		dest["Sunny Skies Park"].buffer_order = B_FIFO;
		dest["Sunny Skies Park"].position( 456, 412 );

		dest["Joyless Park"].buffer_order = B_FIFO;
		dest["Joyless Park"].position( 1361, 424 );

		dest["Narrow Path Park"].buffer_order = B_LIFO;
		dest["Narrow Path Park"].position( 1162, 78 );

		dest["Auctioneer School"].arrival_function = &auctioneer_school;
		dest["Auctioneer School"].position( 246, 856 );

		dest["Little League Field"].arrival_function = &little_league_field;
		dest["Little League Field"].position( 1267, 711 );

		dest["Tom's Trims"].arrival_function = &toms_trims;
		dest["Tom's Trims"].position( 951, 648 );

		dest["Trunkers"].arrival_function = &trunkers;
		dest["Trunkers"].position( 692, 543 );

		dest["Rounders Pub"].arrival_function = &rounders_pub;
		dest["Rounders Pub"].position( 1063, 482 );

		dest["Fueler Up"].gas_price = 1.92;
		dest["Fueler Up"].max_passengers = 0;
		dest["Fueler Up"].position( 1155, 557 );

		dest["Go More"].gas_price = 1.75;
		dest["Go More"].max_passengers = 0;
		dest["Go More"].position( 258, 764 );

		dest["Zoom Zoom"].gas_price = 1.45;
		dest["Zoom Zoom"].max_passengers = 0;
		dest["Zoom Zoom"].position( 546, 52 );

		dest["Knots Landing"].arrival_function = &knots_landing;
		dest["Knots Landing"].position( 1426, 314 );

		dest["Bird\'s Bench"].max_passengers = 1;
		dest["Bird\'s Bench"].position( 197, 653 );

		dest["Rob\'s Rest"].max_passengers = 1;
		dest["Rob\'s Rest"].position( 323, 473 );

		dest["Firemouth Grill"].buffer_order = B_RANDOM;
		dest["Firemouth Grill"].position( 770, 440 );

		dest["Cyclone"].arrival_function = &cyclone;
		dest["Cyclone"].position( 272, 314 );

		dest["Chop Suey"].arrival_function = &chop_suey;
		dest["Chop Suey"].position( 1374, 169 );

		dest["The Underground"].arrival_function = &the_underground;
		dest["The Underground"].position( 1182, 462 );

		dest["Collator Express"].arrival_function = &collator_express;
		dest["Collator Express"].position( 424, 351 );

		dest["Crime Lab"].arrival_function = &crime_lab;
		dest["Crime Lab"].position( 1031, 796 );

		dest["Equal\'s Corner"].arrival_function = &equals_corner;
		dest["Equal\'s Corner"].position( 210, 976 );

		// link the nodes together forming the map
		node::street( 9, &dest["Zoom Zoom"], corner[1], inter[1], inter[8], &dest["Cyclone"], inter[9], &dest["Writer\'s Depot"], corner[2], inter[16] );
		node::street( 7, &dest["What\'s The Difference"], inter[1], inter[2], &dest["Riverview Bridge"], inter[3], inter[4], inter[5] );
		node::street( 2, inter[7], inter[3] );
		node::street( 3, inter[7], &dest["Narrow Path Park"], inter[6] );
		node::street( 2, inter[2], &dest["Addition Alley"] );
		node::street( 4, inter[9], inter[10], &dest["Trunkers"], inter[17] );
		node::street( 3, &dest["Rob\'s Rest"], inter[12], &dest["Bird\'s Bench"] );
		node::street( 2, inter[12], inter[11] );
		node::street( 5, inter[16], &dest["Charboil Grill"], &dest["Go More"], inter[13], inter[15] );
		node::street( 9, inter[8], &dest["Collator Express"], &dest["Sunny Skies Park"], inter[10], inter[11], inter[13], &dest["Auctioneer School"], inter[14], &dest["Equal\'s Corner"] );
		node::street( 3, inter[14], &dest["Starchild Numerology"], inter[15] );
		node::street( 3, inter[29], inter[30], &dest["Rounders Pub"] );
		node::street( 3, inter[29], inter[31], &dest["Joyless Park"] );
		node::street( 2, inter[28], inter[27] );
		node::street( 4, inter[27], inter[24], &dest["Multiplication Station"], inter[22] );
		node::street( 3, inter[23], &dest["Little League Field"], inter[24] );
		node::street( 2, inter[26], &dest["Taxi Garage"] );
		node::street( 4, &dest["Divide and Conquer"], inter[32], inter[33], &dest["Knots Landing"] );
		node::street( 9, inter[18], corner[3], inter[17], &dest["Magic Eight"], corner[4], inter[19], inter[21], inter[34], inter[22] );
		node::street( 3, inter[23], corner[6], inter[34] );
		node::street( 5, inter[18], &dest["Firemouth Grill"], inter[20], inter[25], inter[23] );
		node::street( 4, inter[20], &dest["Tom's Trims"], &dest["Post Office"], inter[19] );
		node::street( 14, &dest["The Babelfishery"], inter[21], &dest["Crime Lab"], corner[5], inter[25], inter[26], &dest["Fueler Up"], inter[28], &dest["The Underground"], inter[31], inter[32], &dest["KonKat\'s"], inter[4], inter[6] );
		node::street( 4, inter[5], &dest["Chop Suey"], &dest["Heisenberg\'s"], inter[33] );
	}

	location* get_location( const string& name ) {
		if( !dest.count(name) )
			return NULL;
		return &dest[name];
	}
};

class program
{
	typedef enum { C_NONE, C_WAITING, C_GOTO, C_SWITCH, C_SWITCH_IF, C_PICKUP } command_t;
	typedef struct code_t_ {
		command_t cmd;
		string    loc;
		string    data;
		code_t_() : cmd(C_NONE) {}
		code_t_( const code_t_& c ) : cmd(c.cmd), loc(c.loc), data(c.data) {}
	} code_t;
	vector<code_t> 	   script;
	map<string,size_t> labels;

public:
	program( const string& filename ) {
		ifstream file( filename.c_str() );
		if( !file.is_open() )
			error( "could not open file" );

		string input;
		while( !file.eof() ) {
			string in;
			getline( file, in );
			input += in;
		}

		parse( input );
	}

	void parse( string& in ) {
		vector<string> tokens;
		while( !in.empty() ) {
			// skip spaces
			string::size_type e = in.find_first_not_of(" \t\r\n");
			if( e != string::npos )
				in = in.substr( e );

			// check for labels
			// or check for normal statements (which end in a period)
			if( in[0] == '[' ) {
				string::size_type p = in.find(']');
				if( p != string::npos ) {
					labels[in.substr(1,p-1)] = script.size();
					in.erase( 0, p+1 );
				} else {
					error( "parse error: incomplete label" );
					break;
				}
			} else {
				bool was_quoted = false;
				string tok = get_token( in, was_quoted );
				if( tok.empty() )
					break;
				if( tok == "." && !was_quoted ) {
					compile( tokens );
				} else {
					tokens.push_back( tok );
				}
			}
		}

		if( !tokens.empty() )
			error( "parse error: likely incomplete statement" );
	}

	void compile( vector<string>& in ) { 
		code_t c;
		// is waiting at...
		if( in.size()>=5 && in[1]=="is" && in[2]=="waiting" && in[3]=="at" ) {
			c.cmd = C_WAITING;
			c.data = in[0];
			size_t s = (in[4]=="the")? 5: 4;
			for( size_t p=0; p+s < in.size(); p++ ) {
				if( p ) c.loc += " ";
				c.loc += in[p+s];
			}
		} else if( in.size()>=5 && in[0]=="Go" && in[1] == "to" ) {
			c.cmd = C_GOTO;
			size_t s = (in[2]=="the")? 3: 2;
			size_t p = 0;
			for( ; (p+s < in.size()) && (in[p+s] != ":"); p++ ) {
				if( p ) c.loc += " ";
				c.loc += in[p+s];
			}
			p++;	// skip the colon
			for( ; p+s < in.size(); p++ ) {
				string str = in[p+s];
				int n = strtol( str.c_str(), NULL, 10 );
				if( n > 0 ) c.data += to_string(n) + ":";
				else if( !str.empty() ) c.data += to_upper(str)[0];
			}
		} else if( in.size()>=6 && in[0]=="Pickup" && (in[1]=="a" || in[1]=="another") && in[2]=="passenger" && in[3]=="going" && in[4]=="to" ) {
			c.cmd = C_PICKUP;
			size_t s = (in[5]=="the")? 6: 5;
			for( size_t p=0; p+s < in.size(); p++ ) {
				if( p ) c.loc += " ";
				c.loc += in[p+s];
			}
		} else if( in.size()>=4 && in[0]=="Switch" && in[1]=="to" && in[2]=="plan" ) {
			c.cmd = (in.size()>4)? C_SWITCH_IF: C_SWITCH;
			c.data = in[3];
		} else {
			string err = "parse error near:";
			for( size_t p=0; p<in.size(); p++ ) {
				err += " ";
				err += in[p];
			}
			error( err.c_str() );
		}

		//cout << "debug: " << c.cmd << " \"" << c.data << "\" " << c.loc << endl;
		script.push_back( c );
		in.clear();
	}

	string get_token( string& in, bool& was_quoted ) {
		string r;
		string::size_type p;
		was_quoted = false;
		if( !in.empty() ) {
			char c = in[0];
			if( (c=='\'' || c=='"') && string::npos != (p=in.find(c,1)) ) {
				r = in.substr( 1, p-1 );
				in.erase( 0, r.size()+2 );
				was_quoted = true;
			} else if( c == '.' || c == ':' ) {
				r = c;
				in.erase( 0, 1 );
			} else {
				r = in.substr( 0, in.find_first_of(" \r\n\t.:") );
				in.erase( 0, r.size() );
			}
		}
		return r;
	}

	void run( road_map& the_map ) {
		// 1 mile:			264 pixels
		// Max gallons of gas:		20
		// Starting gallons:		20
		// miles per gallon:		18
		// starting credits:		0
		// fare in credits per mile:	0.07
		taxi car( the_map.get_location("Taxi Garage"), 264, 20, 20, 18.0, 0, 0.07 );

		// now let's do it!
		for( size_t i=0; i<script.size(); i++ ) {
			code_t* c = &script[i];
			location* loc = the_map.get_location( c->loc );

			//cout << "debug2: " << c->cmd << " \"" << c->data << "\" " << c->loc << " gas: " << car.gas << " credits: " << car.credits << " miles: " << car.miles_driven << endl;

			if( c->cmd == C_WAITING ) {
				if( !loc ) {
					error( "missing location for waiting statement" );
					continue;
				}
				loc->waiting( c->data.c_str() );
			} else if( c->cmd == C_PICKUP ) {
				if( !loc ) {
					error( "missing destination in pickup statement" );
					continue;
				}
				car.pickup_passenger( loc );
			} else if( c->cmd == C_GOTO ) {
				if( !loc ) {
					error( "missing destination in go to statement" );
					continue;
				}
				if( c->data.empty() ) {
					error( "invalid directions in go to statement" );
					continue;
				}
				string path = c->data;
				switch( path[0] ) {
				case 'N':	car.set_direction( D_NORTH );	break;
				case 'E':	car.set_direction( D_EAST );	break;
				case 'S':	car.set_direction( D_SOUTH );	break;
				case 'W':	car.set_direction( D_WEST );	break;
				default: {
						error( "invalid cardinal direction" );
						continue;
					}
				}
				path.erase( 0, 1 );
				while( !path.empty() ) {
					int turns = strtol( path.c_str(), NULL, 10 );
					if( turns < 1 ) break;
					string::size_type p = path.find(':');
					if( p == string::npos || p+1 == path.size() ) break;
					path = path.substr( p+1 );
					rel_dir_t d = (path[0]=='L')? D_LEFT: D_RIGHT;
					path.erase( 0, 1 );
					car.take_xth_turn( turns, d );
				}
				if( !path.empty() ) {
					error( "invalid directions in go to statement" );
					continue;
				}
				car.drive_to( loc );
			} else if( c->cmd == C_SWITCH ) {
				if( c->data.empty() ) {
					error( "missing label in switch command" );
					continue;
				}
				if( !labels.count(c->data) ) {
					error( "no such label" );
					continue;
				}
				i = labels[c->data] - 1;
			} else if( c->cmd == C_SWITCH_IF ) {
				if( c->data.empty() ) {
					error( "missing label in switch_if command" );
					continue;
				}
				if( !labels.count(c->data) ) {
					error( "no such label" );
					continue;
				}
				if( !car.current_location ) {
					error( "cannot switch, not at a passenger destination" );
					continue;
				}
				if( !car.current_location->outgoing_passengers() ) {
					i = labels[c->data] - 1;
				}
			}
		}
	}
};

int main( int argc, char** argv )
{
	if( argc <= 1 ) {
		cerr << "usage: " << argv[0] << " script.txt" << endl << endl;
		exit( -1 );
	}

	cout << "Welcome to Taxi!" << endl;
	cout << "Let the journey begin..." << endl << endl;
	srandom( time(NULL) );

//-------------------------------------------------------------------------------------------

	road_map the_map;
	program pgm( argv[1] );
	pgm.run( the_map );

//-------------------------------------------------------------------------------------------

	error( "The boss couldn't find your taxi in the garage.  You're fired!" );
	return -1;
}

