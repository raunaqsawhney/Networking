#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>

#include "es.h"

#define H = 54
#define l = 1500

void do_abp(params_t params) 
{
	int SN = 0;
	int NEXT_EXPECTED_ACK = 1;
	int delta_timeout = params.delta_timeout;				
	int current_time = 0;	//t_c
	int packet_size = params.frame_header_len + params.packet_len;
	int sender_buffer_size = 1;

	double transmission_delay = params.packet_len / params.link_rate;	// L/C

	packet_t frame;
	packet_t sender_buffer[1];

	struct event *packet_received;


	/*
	 * SIMULATE until the number of successfully delivered packets is not reached
	 */
	while (success_count != params.duration)
	{
		// Create new packet
		frame.size = params.frame_header_len + params.packet_len;
		frame.sequence_number = SN;

		// Add new packet to buffer
		sender_buffer[0] = frame;

		// Send packet to channel
		register_event('t', current_time + transmission_delay + delta_timeout);
		packet_received = send(params, frame);

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

bool gen_rand(double probability)
{
    return rand() <  probability * ((double)RAND_MAX + 1.0);
}

struct event * send(params_t params, packet_t frame)
{
	// Send should return an ACK event, but may not if there are errors
	// The ACK event must have a sequence number RN, error-flag, and type 

	int frame_length = frame.size;
	double ber = params.ber;
	bool rand_num = false;
	int zero_count = 0;

	int i; 

	struct event *send_back;

	for (i = 0; i < frame_length; i++)
	{
		rand_num = gen_rand(ber);
		if (rand_num == 0)
			zero_count++;
	}

	if (zero_count == 0)
	{
		// Correct frame
		send_back->val = frame->val + tau;
		send_back->error_flag = 0;
		send_back->sequence_number = frame->sequence_number;
	}
	else if (zero_count >= 1 && zero_count <= 4)
	{
		// Frame/ACK Error
		send_back->val = frame->val + tau;
		send_back->error_flag = 1;
		send_back->sequence_number = frame->sequence_number;
	}
	else if (zero_count >= 5) 
	{
		// LOSS
		send_back = NULL;

	}

}



struct event * list_init_event(char type, double time)
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

void list_insert_event(struct event *new, struct event *from)
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
		temp = from;
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
			prev = from;
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


int main(int argc, char *argv[]) 
{
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
