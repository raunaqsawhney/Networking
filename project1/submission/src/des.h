#ifndef DES_H_
#define DES_H_


/* EVENT PACKET */
struct event {
    char type;
    float val;
    float len;
    struct event* next;
};

struct result {
	int duration;
	float alpha;
	float lambda;
	int packet_len;
	int link_rate;
	int buffer_size;
	float roh;
	double total_time;
	int num_observations;
	int num_packet_arrivals;
	int num_packet_departures;
	long double num_packets_in_system;
	float avg_packets_in_system;
	int num_idle;
	float probability_idle;
	int num_dropped;
	float probability_dropped;
};


/* 
 *  Discrete Event Simulation Functions
 */
 void gen_observers(int , int );
 void gen_arrivals(float , int , int );
 void compute_metrics(long double , int , int, int, int, int, int );
 void add_to_queue(struct event *);

 void run_system(int , int);

 #endif