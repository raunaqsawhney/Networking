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
#define N 4
#define C 5000000
#define DURATION 1000

double tau = 0.0;
double ber = 0.0;
double delta_timeout = 0.0;
double throughput = 0.0;

struct event *first = (struct event *) NULL;
struct event *last = (struct event *) NULL;

int SN[N] = {};
double T[N] = {};
int NEXT_EXPECTED_ACK[N] = {};
int NEXT_EXPECTED_FRAME = 0;

double T_c = 0.0;
int counter = 0;

double transmission_delay = 0.0;
double header_transmission_delay = 0.0;
int frame_length = 0;
int success_count = 0;

int P = 0;

struct event *timeout_ptr = (struct event *)NULL;

int gen_rand(double probability)
{
    double rndDouble = (double)rand() / RAND_MAX;

    if (rndDouble < probability)
    	return 0;
    else
    	return 1;
}

void print_event(struct event *ptr)
{
	printf("Type:\t%c\n", ptr->type);
	printf("Time:\t%f\n", ptr->val);
	printf("\n");
}

void print_list(struct event *ptr)
{
	while (ptr != NULL)
	{
		print_event(ptr);
		ptr = ptr->next;
	}
}

struct event * list_init_event(char type, double val, int error_flag, int sequence_number)
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
		ptr->error_flag = error_flag;
		ptr->sequence_number = sequence_number;

		if (type == 't')
			timeout_ptr = ptr;

		return ptr;
	}

	free(ptr);
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

void print_buffer_sn()
{
	printf("PRINT BUFFER SN\n");
	int i;
	for (i = 0; i < N; i++)
	{
		printf("[%d] - %d\n", i, SN[i]);
	}
}

void print_buffer_t()
{
	printf("PRINT BUFFER T\n");
	int i;
	for (i = 0; i < N; i++)
	{
		printf("[%d] - %f\n", i, T[i]);
	}
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

void purge_timeout (struct event *ptr)
{
	printf("BEFORE PURGE\n");
	print_list(first);

	struct event *temp;

	if (first == NULL)
   		return; 

	if (ptr == first && ptr->type == 't') { 
		printf("TO PURGE:\n");
		print_event(first); 
		list_delete_event(first);         
	}
    else
    {
    	temp = first;          
       	while(temp->type != 't') {
       		temp = temp->next;
       		if (!temp)
       			break;
       	}

       	if (temp != NULL)
		{	
	       	printf("TO PURGE:\n");
	       	print_event(temp);
			list_delete_event(temp);   
       	}                  
   	}
   	printf("AFTER PURGE\n");
	print_list(first); 
}

int check_RN(int sequence_number)
{
	int i;
	for (i = 0; i < N; i++)
	{
		if (sequence_number == NEXT_EXPECTED_ACK[i])
		{
			printf("CHECK_RN -- RN = %d | NEA[%d] = %d\n", sequence_number, i, NEXT_EXPECTED_ACK[i]);
			return 1;
		}
	}
	return 0;
}

void window_slide(int window_slide_amount)
{
	success_count += window_slide_amount;
	counter -= window_slide_amount;
	
	// Shift the counter left by some amount
	int i;

	printf("BEFORE\n");
	printf("------SN------\n");
	for (i = 0; i < N; i++)
	{
		printf("%d\n", SN[i]);
	}

	printf("------T------\n");
	for (i = 0; i < N; i++)
	{
		printf("%f\n", T[i]);
	}

	printf("------NEA------\n");
	for (i = 0; i < N; i++)
	{
		printf("%d\n", NEXT_EXPECTED_ACK[i]);
	}

	printf("\n");

	i = 0;
	for (i = window_slide_amount; i < N; i++)
	{
		SN[i - window_slide_amount] = SN[i];
		T[i - window_slide_amount] = T[i];
		NEXT_EXPECTED_ACK[i - window_slide_amount] = NEXT_EXPECTED_ACK[i];
	}

	printf("AFTER\n");
	printf("------SN------\n");
	for (i = 0; i < N; i++)
	{
		printf("%d\n", SN[i]);
	}

	printf("------T------\n");
	for (i = 0; i < N; i++)
	{
		printf("%f\n", T[i]);
	}

	printf("------NEA------\n");
	for (i = 0; i < N; i++)
	{
		printf("%d\n", NEXT_EXPECTED_ACK[i]);
	}
}

void register_event(char type, double val, int error_flag, int sequence_number)
{
	struct event *ptr;

	// Initialize event with type and time
	ptr = list_init_event(type, val, error_flag, sequence_number);

	// Insert the event into the ES
	list_insert_event(ptr);
}

void initialize()
{
	T_c = 0.0;
	counter = 0;
	SN[0] = 0;
	NEXT_EXPECTED_FRAME = 0;
	// NEXT_EXPECTED_ACK ALREADY INITIALIZED AS EMPTY LIST
}

frame_t channel(int sequence_number, double T, int L)
{
	int rand_num = 0;					// 0 or 1
	int zero_count = 0;					// 0 counter for bits in error
	frame_t channel_output;

	int i; 

	// Forward Channel
	for (i = 0; i < frame_length; i++)
	{
		rand_num = gen_rand(ber);
		if (rand_num == 0)
			zero_count++;
	}

	if (zero_count == 0)
	{
		// Correct frame
		channel_output.val = T + tau;
		channel_output.error_flag = 0;
		channel_output.sequence_number = sequence_number;
		channel_output.is_null = 0;
		channel_output.size = -1;
	}
	else if (zero_count >= 1 && zero_count <= 4)
	{
		// Frame/ACK Error
		channel_output.val = T + tau;
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

frame_t receiver(int sequence_number, double T, int error_flag)
{
	frame_t ack;
	int forward_channel_error = error_flag;

	if (forward_channel_error == 0 && sequence_number == NEXT_EXPECTED_FRAME)
	{
		NEXT_EXPECTED_FRAME = (NEXT_EXPECTED_FRAME + 1) % (N + 1);
		// New and successfully delivered packet
		// Send to layer 3
	} 

	ack.size = H;
	ack.sequence_number = NEXT_EXPECTED_FRAME;
	ack.error_flag = -1;
	ack.val =  T + header_transmission_delay;
	return ack;
}

void send(int SN, double T) 
{
	frame_t frame_in_transmission;
	frame_t packet_received;

	printf("INPUT TO F_CHANNEL:\tTime = %f\tSN = %d\tL = %d\n", T, SN, frame_length);
	frame_in_transmission = channel(SN, T, frame_length);
	
	if (frame_in_transmission.is_null)
	{
		packet_received.is_null = 1;
	}

	printf("INPUT TO RECEIVER:\tTime = %f\tSN = %d\tERROR = %d\tIS_NULL = %d\n", frame_in_transmission.val, frame_in_transmission.sequence_number, frame_in_transmission.error_flag, frame_in_transmission.is_null);
	frame_in_transmission = receiver(frame_in_transmission.sequence_number, frame_in_transmission.val, frame_in_transmission.error_flag);
	
	if (frame_in_transmission.is_null)
	{
		packet_received.is_null = 1;
	}

	printf("INPUT TO R_CHANNEL:\tTime = %f\tRN = %d\tL = %d\n", frame_in_transmission.val, frame_in_transmission.sequence_number, H);
	frame_in_transmission = channel(frame_in_transmission.sequence_number, frame_in_transmission.val, H);

	if (!frame_in_transmission.is_null)
	{
		packet_received = frame_in_transmission;
		printf("Registered an ACK Event AT %f\n", packet_received.val);
		register_event('a', packet_received.val, packet_received.error_flag, packet_received.sequence_number);
	}
}

void sender()
{
	printf("SENDER\n");
	struct event *next_event;
	int window_slide_amount = 0;

	printf("COUNTER BEFORE WHILE: %d\n", counter);
	while (counter < N)
	{
		printf("[S]SUCCESS COUNT: %d\n", success_count);
		if (success_count == DURATION)
		{
			printf("BREAK\n");
			break;
		}

		printf("COUNTER: %d | N: %d\n", counter, N);
		T_c = T_c + transmission_delay;
		T[counter] = T_c;											// Transmission Time
		NEXT_EXPECTED_ACK[counter] = (SN[counter] + 1) % (N + 1);

		if (counter == 0)
		{
			register_event('t', T[counter] + delta_timeout, -1, -1);
		}

		send(SN[counter], T[counter]);
		next_event = first;

		if ((next_event->val < T[counter]) && (next_event->type == 't'))
		{
			printf("CONDITION 1\n");
			purge_timeout(timeout_ptr);
			counter = 0;
			continue;
		}
		
		else if (next_event->val < T[counter] && next_event->error_flag == 0 && check_RN(next_event->sequence_number) && next_event->type == 'a')
		{
			printf("CONDITION 2\n");
			P = SN[0];
			window_slide_amount = (next_event->sequence_number - P + N + 1) % (N + 1);
			window_slide(window_slide_amount);
			purge_timeout(timeout_ptr);
			register_event('t', T[0] + delta_timeout, -1, -1);
		}

		if ((counter + 1) < N)
		{
			counter = counter + 1;
			SN[counter] = (SN[counter - 1] + 1) % (N + 1);
		} 
		else 
		{
			break;
		}
		// counter = counter + 1;
		// SN[counter] = (SN[counter - 1] + 1) % (N + 1);
	}
	print_buffer_sn();
	print_buffer_t();
}

int is_empty()
{
	return !first;
}

void event_processor()
{
	printf("------- EVENT PROCESSOR --------\n");

	struct event *next_event;
	int window_slide_amount = 0;
	print_list(first);

	while (!is_empty())
	{
		printf("[E]SUCCESS COUNT: %d\n", success_count);
		if (success_count == DURATION)
		{
			printf("BREAK\n");
			break;
		}

		next_event = read_es();
		T_c = next_event->val;
		printf("T_c: %f\n", T_c);

		if (next_event->type == 't')
		{
			printf("CONDITION 4\n");
			counter = 0;
			purge_timeout(timeout_ptr);
			sender();
		}
		else if (next_event->type == 'a' && !next_event->error_flag && check_RN(next_event->sequence_number))
		{
			printf("CONDITION 5\n");

			P = SN[0];
			window_slide_amount = (next_event->sequence_number - P + N + 1) % (N + 1);
			window_slide(window_slide_amount);
			purge_timeout(timeout_ptr);
			register_event('t', T[0] + delta_timeout, -1, -1);
			
			if ((counter + 1) < N)
			{
				counter = counter + 1;
				SN[counter] = (SN[counter - 1] + 1) % (N + 1);
			} 
			else 
			{
				break;
			}

			sender();
		}
	}
}

int main(int argc, char *argv[]) 
{
	srand(time(0));	

	tau = 0.005;
	ber = 0.0001;
	delta_timeout = 2.5*tau;

	transmission_delay = ((double)H + (double)l)/(double)C;
	header_transmission_delay = (double)H/(double)C;

	frame_length = H + l;

	initialize();
	sender();
	event_processor();

	throughput = ((double)DURATION * ((double)l))/T_c;
	
	printf("\n");
	printf("T_C: %f | Throughput: %f\n", T_c, throughput);

}