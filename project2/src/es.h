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

typedef struct frame {
	char type;
	double val;
	int error_flag;
	int sequence_number;
	int size;
	int is_null;
} frame_t;

// PARAMS STRUCT
typedef struct params {

	// Sender parameters
	int frame_header_len;
	int packet_len;
	double delta_timeout;

	// Channel parameters
	int link_rate;
	double tau;
	double ber;

	// Experiment parameters
	int duration;
} params_t;

typedef struct success {
	int is_success;
	double current_time;
	int SN;
	int frame_length;
	int NEXT_EXPECTED_ACK;
} success_t;

success_t check_next_event(struct event *);
struct event * read_es();	
success_t do_send();
frame_t send();
frame_t channel(int, int);
frame_t receiver(int, int);


#endif
