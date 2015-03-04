#ifndef DES_H_
#define DES_H_


// EVENT STRUCT 
struct event {
    char type;					// Time out (TO) or Acknowledgement (ACK)
    double val;
    int error_flag;
    int sequence_number;
    struct event* next;
};

struct packet {
	int size;
	int sequence_number
} packet_t;

// PARAMS STRUCT
struct params {

	// Sender parameters
	int frame_header_len;
	int packet_len = l
	int delta_timeout;

	// Channel parameters
	int link_rate;
	int tau;
	double ber;

	// Experiment parameters
	int duration;
} params_t;

/*
 * Adds an event to the ES
 * New event inserted at the right position (based on its time field)
 */
void register_event(struct event *);

struct event * dequeue();
void send()
void purge_time_out();

#endif
