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

	// For GBN Buffer
	int N;
} params_t;

typedef struct success {
	int is_success;
	double current_time;
	int SN;
	int frame_length;
	int NEXT_EXPECTED_ACK;
	int num_success_packets;
} success_t;

struct gbn_frame {
    int index;
    int count;
    int sequence_number;
    int frame_length;
    double current_time;
    struct gbn_frame* next;
};

success_t check_next_event(struct event *);
struct event * read_es();	
frame_t send_abp();
frame_t channel_abp(int, int);
frame_t receiver_abp(int, int);

success_t do_send();

#endif
