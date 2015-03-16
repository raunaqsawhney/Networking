#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#include "es.h"

FILE *fp;

#define H 54*8
#define l 1500*8
#define C 5000000
#define D 10000
#define BER_THRESHOLD 5

params_t params;

// Create a new frame
frame_t frame;

// Create a new success struct
success_t success;

frame_t sender_buffer[1] = {};
frame_t tmp_loc_for_packet_received;
struct event *next_event_deleted;

// ES Linked List
struct event *first = (struct event *) NULL;
struct event *last = (struct event *) NULL;

// Globally Initialize Sender
int SN = 0;
int NEXT_EXPECTED_ACK = 1;
int NEXT_EXPECTED_FRAME = 0;
double CURRENT_TIME = 0.0;
int frame_length;
double transmission_delay;	// L/C

void reinit_global_vars()
{
	frame_t sender_buffer[1] = {};
	first = NULL;
	last = NULL;

	// Globally Initialize Sender
	SN = 0;
	NEXT_EXPECTED_ACK = 1;
	NEXT_EXPECTED_FRAME = 0;
	CURRENT_TIME = 0.0;
}

void print_event(struct event *ptr)
{

	printf("Type:\t%c\n", ptr->type);
	printf("Time:\t%f\n", ptr->val);
}

void print_list(struct event *ptr)
{
	while (ptr != NULL)
	{
		print_event(ptr);
		ptr = ptr->next;
	}
}

struct event * list_init_event(char type, double val)
{
	struct event *ptr;
	ptr = (struct event *)malloc(sizeof(struct event));
	if (ptr == NULL)
	{
		return (struct event *) NULL;
	}
	else
	{
		ptr->type = type;
		ptr->val = val;
		return ptr;
	}
	free(ptr);
}

void list_add_event(struct event *new)
{
	
	if (first == NULL)
	{
		first = new;
	}
	else
	{
		last->next = new;
	}
	new->next = NULL;
	last = new;
}

void list_insert_event(struct event *new)
{

	struct event *temp, *prev;

	// If the List is empty, add the new event to the start 
	if (first == NULL)
	{
		first = new;
		last = new;
		first->next = NULL;
		return;
	}
	else 
	{
		temp = first;
		while (temp->val <= new->val)
		{
			temp = temp->next;
			if (temp == NULL)
			{
				break;
			}
		}

		if (temp == first) 
		{
			new->next = first;
			first = new;
		}
		else
		{
			prev = first;
			while (prev->next != temp)
			{
				prev = prev->next;
			}
			prev->next = new;
			new->next = temp;
			if (last == prev)
			{
				last = new;
			}
		}
	}
}

void list_delete_event(struct event *ptr)
{
	struct event *temp, *prev;
	temp = ptr;
	prev = first;

	if (temp == prev)
	{
		first = first->next;
		if (last == temp)
		{
			last = last->next;
		}
		free(temp);

	}
	else
	{
		while (prev->next != temp)
		{
			prev = prev->next;
		} 
		prev->next = temp->next;
		if (last == temp)
		{
			last = prev;
		}
		free(temp);
	}
}

void list_cleanup( struct event *ptr )
{

   struct event *temp;

   if( first == NULL ) return; 

   if( ptr == first ) {      
       first = NULL;         
       last = NULL;          
   }
   else {
       temp = first;          
       while( temp->next != ptr )         
           temp = temp->next;
       last = temp;                        
   }

   while( ptr != NULL ) {   
      temp = ptr->next;     
      free( ptr );          
      ptr = temp;           
   }
}

void register_event(char type, double val)
{
	struct event *ptr;

	// Initialize event with type and time
	ptr = list_init_event(type, val);

	// Insert the event into the ES
	list_insert_event(ptr);
}

int gen_rand(double probability)
{

    double rndDouble = (double)rand() / RAND_MAX;

    if (rndDouble < probability)
    	return 0;
    else
    	return 1;
}

double do_abp_nak() 
{
	double throughput = 0.0;

	int success_count = 0;

	//SIMULATE until the number of successfully delivered packets is not reached
	while (success_count != params.duration)
	{
		// Initialize a frame with size and sequence number (SN = 0)
		frame.size = frame_length;
		frame.sequence_number = SN;

		#ifdef DEBUG
			printf("[%d] SN: %d NEA: %d CT: %f FS: %d\n", success_count, frame.sequence_number, NEXT_EXPECTED_ACK, CURRENT_TIME, frame.size);
		#endif

		// Store a new packet in sender's (only) buffer
		sender_buffer[0] = frame;
		#ifdef DEBUG
			printf("SN IN BUFF: %d\n", frame.sequence_number);
		#endif

		success = do_send();

		if (success.is_success)
		{
			success_count++;
		}
		else 
		{
			printf("ACK HAD ERRORS, but not LOST\n");
			
			CURRENT_TIME = next_event_deleted->val;
			continue;
		}
		
		throughput = ((double)success_count * ((double)params.packet_len))/CURRENT_TIME;
		
		printf("SUCCESS COUNT: %d\tPACKET_LEN: %d\tCURRENT_TIME: %f\n", success_count, params.packet_len, CURRENT_TIME);
		printf("THROUGHPUT:\t%f\n", throughput);

	}
	return throughput;
}

success_t do_send()
{
	success_t success; 

	// Send packet to channel
	CURRENT_TIME = CURRENT_TIME + transmission_delay;
	frame_t packet_received = send();

	tmp_loc_for_packet_received = packet_received;
	
	#ifdef DEBUG
		printf("RN: %d, ERROR: %d, IS_NULL: %d, SIZE: %d, TIME: %f\n", packet_received.sequence_number, packet_received.error_flag, packet_received.is_null, packet_received.size, CURRENT_TIME);
	#endif

	if (!packet_received.is_null)
	{
		register_event('a', packet_received.val);
	}

	struct event *next_event;
	next_event = read_es();

	success = check_next_event(next_event);

	return success;
}

frame_t send()
{
	// Send should return an ACK event, but may not if there are errors
	// The ACK event must have a sequence number RN, error_flag, and type 
	
	list_cleanup(first);

	register_event('t', CURRENT_TIME + params.delta_timeout);

	frame_t frame_in_send;
	frame_t send_output;

	#ifdef DEBUG
		printf("INPUT TO F_CHANNEL:\tTime = %f\tSN = %d\tL = %d\n", CURRENT_TIME, SN, frame_length);
	#endif
	frame_in_send = channel(SN, frame_length);
	
	#ifdef DEBUG
		printf("INPUT TO RECEIVER:\tTime = %f\tSN = %d\tERROR = %d\tIS_NULL = %d\n", CURRENT_TIME, frame_in_send.sequence_number, frame_in_send.error_flag, frame_in_send.is_null);
	#endif

	// Check for NULL Frame
	if (frame_in_send.is_null)
	{
		send_output.is_null = 1;
		return send_output;
	}

	frame_in_send = receiver(frame_in_send.sequence_number, frame_in_send.error_flag);
	#ifdef DEBUG
		printf("INPUT TO R_CHANNEL:\tTime = %f\tRN = %d\tH = %d\n", CURRENT_TIME, frame_in_send.sequence_number, H);
	#endif

	frame_in_send = channel(frame_in_send.sequence_number, H);
	#ifdef DEBUG
		printf("INPUT TO SENDER:\tTime = %f\tRN = %d\tERROR = %d\tIS_NULL = %d\n", CURRENT_TIME, frame_in_send.sequence_number, frame_in_send.error_flag, frame_in_send.is_null);
	#endif

	// Check for NULL ACK
	if (frame_in_send.is_null)
	{
		send_output.is_null = 1;
		return send_output;
	}

	// At this point, we are sure we have an ACK
	// Return it, and register with ES
	send_output.is_null = 0;
	send_output = frame_in_send;

	return send_output;

}

frame_t channel(int sequence_number, int frame_length)
{
	int rand_num = 0;					// 0 or 1
	int zero_count = 0;					// 0 counter for bits in error

	frame_t channel_output;
	int i; 

	// Forward Channel
	for (i = 0; i < frame_length; i++)
	{
		rand_num = gen_rand(params.ber);
		if (rand_num == 0)
			zero_count++;
	}

	if (zero_count == 0)
	{
		// Correct frame
		channel_output.val = CURRENT_TIME + params.tau;
		CURRENT_TIME = channel_output.val;

		channel_output.error_flag = 0;
		channel_output.sequence_number = sequence_number;
		channel_output.is_null = 0;
		channel_output.size = -1;

	}
	else if (zero_count >= 1 && zero_count <= 4)
	{
		// Frame/ACK Error
		channel_output.val = CURRENT_TIME + params.tau;
		CURRENT_TIME = channel_output.val;

		channel_output.error_flag = 1;
		channel_output.sequence_number = sequence_number;
		channel_output.is_null = 0;
		channel_output.size = -1;

	}
	else if (zero_count >= BER_THRESHOLD) 
	{
		// LOSS
		channel_output.is_null = 1;
	}

	return channel_output;
}

frame_t receiver(int sequence_number, int error_flag)
{
	int RN = 0;
	int forward_channel_error = error_flag;

	printf("ERROR FLAG IN RECEIVER == %d\n", error_flag);

	if (forward_channel_error == 0 && sequence_number == NEXT_EXPECTED_FRAME)
	{
		NEXT_EXPECTED_FRAME = (NEXT_EXPECTED_FRAME + 1) % 2;
		// New and successfully delivered packet
		// Send to layer 3
	} 

	frame_t ack;
	ack.size = params.frame_header_len;

	// NEXT_EXPECTED_FRAME will be new value if no error, else will be old value
	RN = NEXT_EXPECTED_FRAME;
	ack.sequence_number = RN;

	CURRENT_TIME = CURRENT_TIME + ((double)params.frame_header_len/(double)params.link_rate);
	ack.error_flag = -1;

	return ack;
}

struct event * read_es()
{
	success_t success;

	// Read the ES
	printf("READ ES:\n");
	print_list(first);

	struct event *next_event = first;
	struct event *next_event_tmp = next_event;

	printf("NEXT EVENT WAS:\n");
	print_event(next_event_tmp);
	
	list_delete_event(next_event);

	printf("READ ES AFTER DELETE TOP:\n");
	print_list(first);

	next_event_deleted = next_event_tmp;

	return next_event_tmp;
}

success_t check_next_event(struct event *next_event)
{
	success_t success;

	if (next_event->type == 'a')
	{
		success_t ack_success; 
		if (tmp_loc_for_packet_received.error_flag == 0 && tmp_loc_for_packet_received.sequence_number == NEXT_EXPECTED_ACK)
		{
			// Packet that was received from send() is error free and is the expected ACK. GOOD.
			SN = (SN + 1) % 2;
			NEXT_EXPECTED_ACK = (NEXT_EXPECTED_ACK + 1) % 2;

			ack_success.SN = SN;
			ack_success.NEXT_EXPECTED_ACK = NEXT_EXPECTED_ACK;
			ack_success.current_time = tmp_loc_for_packet_received.val;
			CURRENT_TIME = ack_success.current_time;

			ack_success.is_success = 1;

			success = ack_success;
		}

		else if (tmp_loc_for_packet_received.error_flag == 1 || tmp_loc_for_packet_received.sequence_number != NEXT_EXPECTED_ACK)
		{
			// DO NOTHING
			success.is_success = 0;
		}
	} 
	else if (next_event->type == 't')
	{
		//Retransmit
		success_t retransmit_success;
		while (retransmit_success.is_success != 1)
		{
			retransmit_success = do_send();
		}

		retransmit_success.is_success = 1;
		success = retransmit_success;
	}

	return success;
}

int main(int argc, char *argv[]) 
{
	srand(time(0));
	fp = fopen("ABP_NAK.csv", "a+");
	
	double throughput = 0.0;

	params.frame_header_len = H;
	params.packet_len = l;
	params.link_rate = C;
	params.duration = D;

	frame_length = params.frame_header_len + params.packet_len;
	transmission_delay = (double)frame_length / (double)params.link_rate;	// L/C

	double tau_set[2] = {0.005, 0.25};
	double delta_set[10] = {2.5*tau_set[0], 5*tau_set[0], 7.5*tau_set[0], 10*tau_set[0], 12.5*tau_set[0], 2.5*tau_set[1], 5*tau_set[1], 7.5*tau_set[1], 10*tau_set[1], 12.5*tau_set[1]};
	double ber_set[3] = {0.0, 0.00001, 0.0001};


	//a1
	params.tau = tau_set[0];
	params.delta_timeout = delta_set[0];
	params.ber = ber_set[0];
	throughput = do_abp_nak();
	fprintf(fp, "%f," , throughput);
	throughput = 0.0;
	reinit_global_vars();

	list_cleanup(first);

	//a2
	params.tau = tau_set[0];
	params.delta_timeout = delta_set[0];
	params.ber = ber_set[1];
	throughput = do_abp_nak();
	fprintf(fp, "%f," , throughput);
	throughput = 0.0;
	reinit_global_vars();

	list_cleanup(first);

	// //a3
	params.tau = tau_set[0];
	params.delta_timeout = delta_set[0];
	params.ber = ber_set[2];
	throughput = do_abp_nak();
	fprintf(fp, "%f," , throughput);
	throughput = 0.0;
	reinit_global_vars();

	list_cleanup(first);

	// // //a4
	params.tau = tau_set[1];
	params.delta_timeout = delta_set[5];
	params.ber = ber_set[0];
	throughput = do_abp_nak();
	fprintf(fp, "%f," , throughput);
	throughput = 0.0;
	reinit_global_vars();

	// //a5
	params.tau = tau_set[1];
	params.delta_timeout = delta_set[5];
	params.ber = ber_set[1];
	throughput = do_abp_nak();
	fprintf(fp, "%f," , throughput);
	throughput = 0.0;
	reinit_global_vars();

	list_cleanup(first);

	// //a6
	params.tau = tau_set[1];
	params.delta_timeout = delta_set[5];
	params.ber = ber_set[2];
	throughput = do_abp_nak();
	fprintf(fp, "%f\n" , throughput);
	throughput = 0.0;
	reinit_global_vars();

	list_cleanup(first);

	//b1
	params.tau = tau_set[0];
	params.delta_timeout = delta_set[1];
	params.ber = ber_set[0];
	throughput = do_abp_nak();
	fprintf(fp, "%f," , throughput);
	throughput = 0.0;
	reinit_global_vars();

	list_cleanup(first);

	// //b2
	params.tau = tau_set[0];
	params.delta_timeout = delta_set[1];
	params.ber = ber_set[1];
	throughput = do_abp_nak();
	fprintf(fp, "%f," , throughput);
	throughput = 0.0;
	reinit_global_vars();

	list_cleanup(first);

	//b3
	params.tau = tau_set[0];
	params.delta_timeout = delta_set[1];
	params.ber = ber_set[2];
	throughput = do_abp_nak();
	fprintf(fp, "%f," , throughput);
	throughput = 0.0;
	reinit_global_vars();

	list_cleanup(first);

	//b4
	params.tau = tau_set[1];
	params.delta_timeout = delta_set[6];
	params.ber = ber_set[0];
	throughput = do_abp_nak();
	fprintf(fp, "%f," , throughput);
	throughput = 0.0;
	reinit_global_vars();

	list_cleanup(first);

	//b5
	params.tau = tau_set[1];
	params.delta_timeout = delta_set[6];
	params.ber = ber_set[1];
	throughput = do_abp_nak();
	fprintf(fp, "%f," , throughput);
	throughput = 0.0;
	reinit_global_vars();

	list_cleanup(first);

	//b6
	params.tau = tau_set[1];
	params.delta_timeout = delta_set[6];
	params.ber = ber_set[2];
	throughput = do_abp_nak();
	fprintf(fp, "%f\n" , throughput);
	throughput = 0.0;
	reinit_global_vars();

	list_cleanup(first);

	//c1
	params.tau = tau_set[0];
	params.delta_timeout = delta_set[2];
	params.ber = ber_set[0];
	throughput = do_abp_nak();
	fprintf(fp, "%f," , throughput);
	throughput = 0.0;
	reinit_global_vars();

	list_cleanup(first);

	//c2
	params.tau = tau_set[0];
	params.delta_timeout = delta_set[2];
	params.ber = ber_set[1];
	throughput = do_abp_nak();
	fprintf(fp, "%f," , throughput);
	throughput = 0.0;
	reinit_global_vars();

	list_cleanup(first);

	//c3
	params.tau = tau_set[0];
	params.delta_timeout = delta_set[2];
	params.ber = ber_set[2];
	throughput = do_abp_nak();
	fprintf(fp, "%f," , throughput);
	throughput = 0.0;
	reinit_global_vars();

	list_cleanup(first);

	//c4
	params.tau = tau_set[1];
	params.delta_timeout = delta_set[7];
	params.ber = ber_set[0];
	throughput = do_abp_nak();
	fprintf(fp, "%f," , throughput);
	throughput = 0.0;
	reinit_global_vars();

	list_cleanup(first);

	//c5
	params.tau = tau_set[1];
	params.delta_timeout = delta_set[7];
	params.ber = ber_set[1];
	throughput = do_abp_nak();
	fprintf(fp, "%f," , throughput);
	throughput = 0.0;
	reinit_global_vars();

	list_cleanup(first);

	//c6
	params.tau = tau_set[1];
	params.delta_timeout = delta_set[7];
	params.ber = ber_set[2];
	throughput = do_abp_nak();
	fprintf(fp, "%f\n" , throughput);
	throughput = 0.0;
	reinit_global_vars();

	list_cleanup(first);

	//d1
	params.tau = tau_set[0];
	params.delta_timeout = delta_set[3];
	params.ber = ber_set[0];
	throughput = do_abp_nak();
	fprintf(fp, "%f," , throughput);
	throughput = 0.0;
	reinit_global_vars();

	list_cleanup(first);

	//d2
	params.tau = tau_set[0];
	params.delta_timeout = delta_set[3];
	params.ber = ber_set[1];
	throughput = do_abp_nak();
	fprintf(fp, "%f," , throughput);
	throughput = 0.0;
	reinit_global_vars();

	list_cleanup(first);

	//d3
	params.tau = tau_set[0];
	params.delta_timeout = delta_set[3];
	params.ber = ber_set[2];
	throughput = do_abp_nak();
	fprintf(fp, "%f," , throughput);
	throughput = 0.0;
	reinit_global_vars();

	list_cleanup(first);


	//d4
	params.tau = tau_set[1];
	params.delta_timeout = delta_set[8];
	params.ber = ber_set[0];
	throughput = do_abp_nak();
	fprintf(fp, "%f," , throughput);
	throughput = 0.0;
	reinit_global_vars();

	list_cleanup(first);

	//d5
	params.tau = tau_set[1];
	params.delta_timeout = delta_set[8];
	params.ber = ber_set[1];
	throughput = do_abp_nak();
	fprintf(fp, "%f," , throughput);
	throughput = 0.0;
	reinit_global_vars();

	list_cleanup(first);

	//d6
	params.tau = tau_set[1];
	params.delta_timeout = delta_set[8];
	params.ber = ber_set[2];
	throughput = do_abp_nak();
	fprintf(fp, "%f\n" , throughput);
	throughput = 0.0;
	reinit_global_vars();

	list_cleanup(first);

	//e1
	params.tau = tau_set[0];
	params.delta_timeout = delta_set[4];
	params.ber = ber_set[0];
	throughput = do_abp_nak();
	fprintf(fp, "%f," , throughput);
	throughput = 0.0;
	reinit_global_vars();

	list_cleanup(first);

	//e2
	params.tau = tau_set[0];
	params.delta_timeout = delta_set[4];
	params.ber = ber_set[1];
	throughput = do_abp_nak();
	fprintf(fp, "%f," , throughput);
	throughput = 0.0;
	reinit_global_vars();

	list_cleanup(first);

	//e3
	params.tau = tau_set[0];
	params.delta_timeout = delta_set[4];
	params.ber = ber_set[2];
	throughput = do_abp_nak();
	fprintf(fp, "%f," , throughput);
	throughput = 0.0;
	reinit_global_vars();

	list_cleanup(first);


	//e4
	params.tau = tau_set[1];
	params.delta_timeout = delta_set[9];
	params.ber = ber_set[0];
	throughput = do_abp_nak();
	fprintf(fp, "%f," , throughput);
	throughput = 0.0;
	reinit_global_vars();

	list_cleanup(first);

	//e5
	params.tau = tau_set[1];
	params.delta_timeout = delta_set[9];
	params.ber = ber_set[1];
	throughput = do_abp_nak();
	fprintf(fp, "%f," , throughput);
	throughput = 0.0;
	reinit_global_vars();

	list_cleanup(first);

	//e6
	params.tau = tau_set[1];
	params.delta_timeout = delta_set[9];
	params.ber = ber_set[2];
	throughput = do_abp_nak();
	fprintf(fp, "%f\n" , throughput);
	throughput = 0.0;
	reinit_global_vars();

	list_cleanup(first);


	fclose(fp);
}
