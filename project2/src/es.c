#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#include "es.h"

#define H = 54
#define l = 1500
#define abp_sender_buffer_size = 1

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

void do_abp(params_t params) 
{
	int success_count = 0;
	int SN = 0;
	int NEXT_EXPECTED_ACK = 1;
	int delta_timeout = params.delta_timeout;							// delta timeout			
	int current_time = 0;												// t_c
	int frame_size = params.frame_header_len + params.packet_len;		// L = H + l

	double transmission_delay = params.packet_len / params.link_rate;	// L/C
	struct event *packet_received;

	// Create a new frame
	frame_t frame;

	// Create a new ABP Sender Buffer to hold frames
	frame_t sender_buffer[abp_sender_buffer_size];

	/*
	 * SIMULATE until the number of successfully delivered packets is not reached
	 */
	while (success_count != params.duration)
	{
		// Initialize a frame with size and sequence number (SN = 0)
		frame.size = frame_size
		frame.sequence_number = SN;

		// Store the packet in sender's (only) buffer
		sender_buffer[0] = frame

		// Send packet to channel
		register_event('t', current_time + transmission_delay + delta_timeout);
		packet_received = send(params, frame, current_time + transmission_delay);

		// Do something with the the returned frame
		// Register it with ES
		// If NULL event, do not register with ES
		// Else, it is an ACK (with/without error, sequence number RN)

		// Now read the ES to get the next event
		// Update t_c
		// Remove event from ES
		// If next event is Timeout, same frame with same SN is sent to channel in a new frame
		// If next event is ACK without error and RN == NEXT_EXPECTED ACK, sender knows frame received correctly, increment SN and NEXT_EXPECTED_ACK by 1 (mod 2), generate a new packet, and send a new frame
		// As soon as a frme is passed to forward channel (i.e. at tc + L/C), purge outstanding time outs in ES, and register a new timeout at t_c + L/C + delta,

		success_count++;
	}
}

void register_event(type, val)
{
	struct event *ptr;

	// Initialize event with type and time
	ptr = list_init_event(type, val);

	// Insert the event into the ES
	list_insert_event(ptr);
}

float gen_rand(double probability)
{
	return ((float)rand()/(float)(RAND_MAX + 1.0));
}

struct event * send(params_t params, frame_t frame, current_time_transmission_delay)
{
	// Send should return an ACK event, but may not if there are errors
	// The ACK event must have a sequence number RN, error_flag, and type 

	frame_t forward_channel_output;
	forward_channel_output = forward_channel(params, frame, current_time_transmission_delay);

	frame_t receiver_output;
	receiver_output = receiver(params, forward_channel_output);

	frame_t reverse_channel_output;
	reverse_channel_output = reverse_channel(params, frame, current_time_transmission_delay);

}

frame_t forward_channel(params_t params, frame_t frame, int current_time_transmission_delay)
{
	int frame_size = frame.frame_size;	// L
	double ber = params.ber;			// BER
	int rand_num = 0;					// 0 or 1
	int zero_count = 0;					// 0 counter for bits in error

	frame_t forward_channel_output;
	int i; 

	// Forward Channel
	for (i = 0; i < frame_size; i++)
	{
		rand_num = gen_rand(ber);
		if (rand_num == 0)
			zero_count++;
	}

	if (zero_count == 0)
	{
		// Correct frame
		forward_channel_output.val = current_time_transmission_delay + params.tau;
		forward_channel_output.error_flag = 0;
		forward_channel_output.sequence_number = frame.sequence_number;

	}
	else if (zero_count >= 1 && zero_count <= 4)
	{
		// Frame/ACK Error
		forward_channel_output.val = current_time_transmission_delay + params.tau;
		forward_channel_output.error_flag = 1;
		forward_channel_output.sequence_number = frame.sequence_number;
	}
	else if (zero_count >= 5) 
	{
		// LOSS
		forward_channel_output = NULL;
	}

	return forward_channel_output;
}

frame_t receiver(params_t params, frame_t forward_channel_output)
{
	int RN = 0;
	int NEXT_EXPECTED_FRAME = 0;
	int current_time = 0;			// t_cs

	int forward_channel_error = forward_channel_output.error_flag;

	if (forward_channel_error == 0 && forward_channel_output.sequence_number == NEXT_EXPECTED_FRAME)
	{
		// New and successfully delivered packet
		// Send to layer 3
		NEXT_EXPECTED_FRAME = (NEXT_EXPECTED_FRAME + 1) % 2;
	} 

	frame_t ack 
	ack.size = params.frame_header_len;

	RN = NEXT_EXPECTED_FRAME
	ack.sequence_number = RN;
}

frame_t reverse_channel(params_t params, frame_t frame, int current_time_transmission_delay)
{
	int frame_size = frame.frame_size;	// L
	double ber = params.ber;			// BER
	int rand_num = 0;					// 0 or 1
	int zero_count = 0;					// 0 counter for bits in error

	frame_t reverse_channel_output;
	int i; 

	// Reverse Channel
	for (i = 0; i < frame_size; i++)
	{
		rand_num = gen_rand(ber);
		if (rand_num == 0)
			zero_count++;
	}

	if (zero_count == 0)
	{
		// Correct frame
		reverse_channel_output.val = current_time_transmission_delay + params.tau;
		reverse_channel_output.error_flag = 0;
		reverse_channel_output.sequence_number = frame.sequence_number;

	}
	else if (zero_count >= 1 && zero_count <= 4)
	{
		// Frame/ACK Error
		reverse_channel_output.val = current_time_transmission_delay + params.tau;
		reverse_channel_output.error_flag = 1;
		reverse_channel_output.sequence_number = frame.sequence_number;
	}
	else if (zero_count >= 5) 
	{
		// LOSS
		reverse_channel_output = NULL;
	}

	return reverse_channel_output;
}

int main(int argc, char *argv[]) 
{
	srand(time(0));

	params_t params;

	params.frame_header_len = H;
	params.packet_len = l;
	params.delta_timeout = delta_timeout;
	params.link_rate = link_rate;
	params.tau = tau;
	params.ber = ber;
	params.duration = duration;

	// Question 1
	do_abp(params);
	list_cleanup();
}
