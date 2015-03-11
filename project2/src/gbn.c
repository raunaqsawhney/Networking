#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#include "es.h"

#define H 54*8
#define l 1500*8
#define BER_THRESHOLD 5

// Global struct to hold parameters needed throughout the presentation
params_t params;

// Create a new ABP Sender Buffer to hold frames
frame_t sender_buffer[] = {};

frame_t tmp_loc_for_packet_received;

// ES Linked List
struct event *first = (struct event *) NULL;
struct event *last = (struct event *) NULL;

// GBN Buffer (As a LINKED LIST, with top pointing to first item added)
struct gbn_frame *buffer_first = (struct gbn_frame *) NULL;
struct gbn_frame *buffer_last = (struct gbn_frame *) NULL;

// Globally Initialize Sender
int SN = 0;
int NEXT_EXPECTED_ACK = 1;
int NEXT_EXPECTED_FRAME = 0;
double CURRENT_TIME = 0.0;
int frame_length;
double transmission_delay;			// L/C
int P = 0;

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

void print_item(struct gbn_frame *ptr)
{
	printf("INDEX:\t%d\n", ptr->index);
	printf("SN:\t%d\n", ptr->sequence_number);
	printf("LEN:\t%d\n", ptr->frame_length);
	printf("\n");
}

void print_buffer(struct gbn_frame *ptr)
{
	while (ptr != NULL)
	{
		print_item(ptr);
		ptr = ptr->next;
	}
}

struct gbn_frame * init_buffer_item(int index)
{
	struct gbn_frame *ptr;
	ptr = (struct gbn_frame *)malloc(sizeof(struct gbn_frame));
	if (ptr == NULL)
	{
		return (struct gbn_frame *) NULL;
	}
	else
	{
		ptr->index = index;
		ptr->sequence_number = SN;
		ptr->frame_length = frame_length;
		return ptr;
	}
	free(ptr);
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

void add_to_buffer(struct gbn_frame *new)
{
	
	if (buffer_first == NULL)
	{
		buffer_first = new;
	}
	else
	{
		buffer_last->next = new;
	}
	new->next = NULL;
	buffer_last = new;
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

void do_gbn() 
{
	int i = 0;
	struct gbn_frame *frame;

	// Fill buffer with N GBN packets from Layer 3
	for (i = 0; i < params.N; i++)
	{
		frame = init_buffer_item(i);
		SN = (SN + 1) % (params.N + 1);
		add_to_buffer(frame);
	}
	print_buffer(buffer_first);
	P = buffer_first->sequence_number;

	// Buffer of sender full. Now transmit sequentially the N Pakcetsx
}

success_t do_send()
{
	success_t success; 
	struct gbn_frame *frame = buffer_first;

	// Send packet to channel

	while (frame != NULL)
	{
		CURRENT_TIME = CURRENT_TIME + frame->frame_length/params.link_rate;
		frame->current_time = CURRENT_TIME;			// T[i]

		// Send the packet to the chanlle with frame->current_time holding the value of CURRENT_TIME
		frame_t packet_received = send(frame->current_time);
		
		if (!packet_received.is_null)
		{
			register_event('a', packet_received.val);
		}

		struct event *next_event;
		next_event = read_es();

		if (next_event->type != 'a' || next_event->type != 't')
		{
			frame = frame->next;
			continue;
		}
		else if (next_event->type == 'a' || next_event->type == 't')
		{
			// THIS IS THE HARD SHIT RIGHT HERE
			if (next_event->type == 't')
			{
				// Retransmit all packets in the buffer right away
				// Update the times
			}
			else if (next_event->type == 'a')
			{
				if (packet_received.error_flag == 0)
				{
					// ACK HAD NO ERRORS
					// CHECK if packet_received->sequence_number is either of P+1, P+2...P+N (mod N+1)
					int i;
					struct gbn_frame *ptr = buffer_first;
					int RN_to_test = 0;
					int window_sliding_amount = 0;
					int count = 0;

					while (ptr != NULL && count != params.N)
					{
						RN_to_test = (P + count) % (params.N + 1);
						if (packet_received.sequence_number == RN_to_test)
						{
							// Sender knows one or more packets received correctly
							
							// Slide the window in buffer by window_sliding_amount to the left
							window_sliding_amount = (packet_received.sequence_number - P) % (params.N + 1);
							do_window_slide(window_sliding_amount);	

							P = packet_received.sequence_number;
						}
						count++;
						ptr = ptr-next;
					}

					if (packet_received.error_flag == 1)
					{
						// ACK Had Error
						// Check if RN is NOT in P+1,P+2,...P+N mod (N+1)
					}
				}
				else if (SITUATION 2)
				{
					// ACK ERROR OR RN = P
				}
			}
		}
		else if (frame->index == params.N - 1)
		{
			// Current sent packet is the last packet in the current window (Nth packet)
			// Dequeue the earliest event from ES and process it acordingly
		}

	}

	return success;
}

void do_window_slide(window_sliding_amount)
{
	struct gbn_frame *ptr = buffer_first;
	struct gbn_frame *tmp;
	int i = 0;
	int fetch_from = 0;

	for (i = 0; i < window_sliding_amount; i++)
	{
		fetch_from = window_sliding_amount + (i + 1);

		tmp = get_frame_in_buf_for_new_loc(fetch_from);
		
		// Have the pointer to the new buffer head
		// Move it to ith location in buffer
		
		while (ptr->index != i)
		{
			ptr = ptr->next;
		}
		ptr = tmp;
	}
}

struct gbn_frame * get_frame_in_buf_for_new_loc(int fetch_from)
{
	struct gbn_frame *ptr = buffer_first;
	while (ptr->index != fetch_from)
	{
		ptr = ptr->next;
	}
	return ptr;
}

frame_t send(frame_t frame)
{
	// Send should return an ACK event, but may not if there are errors
	// The ACK event must have a sequence number RN, error_flag, and type 
	
	list_cleanup(first);

	register_event('t', frame->current_time + params.delta_timeout);

	frame_t frame_in_send;

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

	// Frame has passed the reverse channel. Time to send it back to sender

	frame_t send_output;

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

int main(int argc, char *argv[]) 
{
	srand(time(0));	
	double throughput = 0.0;

	params.N = 5;

	params.frame_header_len = H;
	params.packet_len = l;
	params.link_rate = 5000000;
	params.duration = 10000;

	params.tau = 0.005;
	params.ber = 0.0;
	params.delta_timeout = 2.5*params.tau;

	frame_length = params.frame_header_len + params.packet_len;
	transmission_delay = (double)frame_length / (double)params.link_rate;	// L/C

	do_gbn();
	//printf("THRU:\t%f\n", throughput);


}
