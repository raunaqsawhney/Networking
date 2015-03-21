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
#define DURATION 50000

int count = 0;
// Create a new ABP Sender Buffer to hold frames
frame_t sender_buffer[4] = {};

frame_t tmp_loc_for_packet_received;

// ES Linked List
struct event *first = (struct event *) NULL;
struct event *last = (struct event *) NULL;

// GBN Buffer (As a LINKED LIST, with top pointing to first item added)
struct gbn_frame *buffer_first = (struct gbn_frame *) NULL;
struct gbn_frame *buffer_last = (struct gbn_frame *) NULL;

// Globally Initialize Sender
int SN = 0;
int RN = 0;
int NEXT_EXPECTED_FRAME = 0;
int NEXT_EXPECTED_ACK = 0;

double CURRENT_TIME = 0.0;

int frame_length = (H + l);
double transmission_delay = (double)(H + l)/(double)C;			

int P = 0;
struct gbn_frame *ptr_to_first_new_frame = (struct gbn_frame *) NULL;

double throughput = 0.0;
double tau = 0.0;
double ber = 0.0;
double delta_timeout = 0.0;

int success_count = 0;

double time_prev_ack = 0.0;
double TIME_OF_FIRST_FRAME_IN_BUFFER = 0.0;
double TIME_OF_FIRST_ACK = 0.0;
struct event *timeout_ptr = (struct event *)NULL;
int first_frame_in_buffer = 0;


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

void print_item(struct gbn_frame *ptr)
{
	printf("COUNT:\t%d\n", ptr->count);
	printf("SN:\t%d\n", ptr->sequence_number);
	printf("LEN:\t%d\n", ptr->frame_length);
	printf("TIME:\t%f\n", ptr->current_time);
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
	count = count + 1;
	struct gbn_frame *ptr;
	ptr = (struct gbn_frame *)malloc(sizeof(struct gbn_frame));
	if (ptr == NULL)
	{
		return (struct gbn_frame *) NULL;
	}
	else
	{
		ptr->count = count;
		ptr->index = index;
		ptr->sequence_number = SN;
		ptr->frame_length = frame_length;
		ptr->current_time = 0.0;
		return ptr;
	}
	free(ptr);
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

void delete_from_buffer(struct gbn_frame *ptr)
{
	struct gbn_frame *temp, *prev;
	temp = ptr;
	prev = buffer_first;

	if (temp == prev)
	{
		buffer_first = buffer_first->next;
		if (buffer_last == temp)
		{
			buffer_last = buffer_last->next;
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
		if (buffer_last == temp)
		{
			buffer_last = prev;
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

void buffer_cleanup( struct gbn_frame *ptr )
{

   struct gbn_frame *temp;

   if( buffer_first == NULL ) return; 

   if( ptr == buffer_first ) {      
       buffer_first = NULL;         
       buffer_last = NULL;          
   }
   else {
       temp = buffer_first;          
       while( temp->next != ptr )         
           temp = temp->next;
       buffer_last = temp;                        
   }

   while( ptr != NULL ) {   
      temp = ptr->next;     
      free( ptr );          
      ptr = temp;           
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

int gen_rand(double probability)
{

    double rndDouble = (double)rand() / RAND_MAX;

    if (rndDouble < probability)
    	return 0;
    else
    	return 1;
}

void purge_timeout (struct event *ptr)
{
	printf("BEFORE PURGE\n");
	print_list(first);

	struct event *temp;

	if (first == NULL)
   		return; 

	if (ptr == first && ptr->type == 't') {  
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

struct event * read_es()
{
	// Read the ES
	printf("\nREAD ES:\n");
	print_list(first);
	struct event *next_event = first;
	struct event *next_event_tmp = next_event;

	struct event *null_event = (struct event *)NULL;

	if (first == NULL)
		return null_event;

	printf("\nNEXT EVENT WAS:\n");
	print_event(next_event_tmp);
	return next_event_tmp;
}

int check_RN (int sequence_number)
{
	int p_array[4] = {};
	int i;
	int expected_sequence = 0;

	for (i = 0; i < 4; i++)
	{
		expected_sequence = (P + (i + 1)) % (N + 1);
		p_array[i] = expected_sequence;
	}

	printf("EXPECTED SEQUENCE LIST\n");
	for (i = 0; i < 4; i++)
	{
		printf("[%d] - %d\t", i, p_array[i]);
	}
	printf("\n");

	for (i = 0; i < 4; i++)
	{
		if (sequence_number == p_array[i])
			return 1;
	}

	return 0;


	// printf ("CHECK_RN RN: %d\n", sequence_number);
	// if (sequence_number == P)
	// {	
	// 	printf("RETURN BAD\n");
	// 	return 0;
	// }
	// else if (sequence_number <= params.N)
	// {
	// 	printf("RETURN GOOD\n");
	// 	return 1;
	// }

	// printf("RETURN UNKNOWN\n");
	// return -1;
}

void window_slide(int window_sliding_amount)
{
	struct gbn_frame *ptr = buffer_first;
	struct gbn_frame *gbn_frame;

	int count = 0;

	while (count < window_sliding_amount)
	{
		delete_from_buffer(ptr);
		ptr = buffer_first;
		count++;
	}

	count = 1;
	// Re-align all indexes
	while (ptr != NULL)
	{
		ptr->index = count;

		count++;
		ptr = ptr->next;
	}
}

// void retransmit_all()
// {
// 	printf("Retransmitting ALL\n");
// 	list_cleanup(first);

// 	success_t success; 

// 	while (success.is_success != 1)
// 	{
// 		success = do_send();
// 	}
// }

void fill_buffer()
{
	int i;
	struct gbn_frame *frame;

	// Fill buffer with N GBN packets from Layer 3
	for (i = 1; i <= N; i++)
	{
		frame = init_buffer_item(i);
		SN = (SN + 1) % (N + 1);
		add_to_buffer(frame);
	}
}

struct gbn_frame * add_new_packets(int amount)
{
	// amount == RN - P
	int i;
	int first_frame = 0;
	struct gbn_frame *ptr_to_first_new_frame;
	struct gbn_frame *gbn_frame;

	for (i = 1; i <= amount; i++)
	{
		printf("ADDING NEW PACKET WITH SN: %d\n", SN);
		gbn_frame = init_buffer_item(N - amount + 1);
		SN = (SN  + 1) % (N + 1);
		add_to_buffer(gbn_frame);

		if (!first_frame)
		{
			ptr_to_first_new_frame = gbn_frame;
			first_frame = 1;
		}
	}

	// Return the pointer to the first new frame inserted
	return ptr_to_first_new_frame;
}

int process_next_event(struct event * next_event)
{
	int window_sliding_amount = 0;
	int process_result = 0;

	if (next_event->type == 't')
	{
		process_result = 0;
	}

	else if (next_event->type == 'a' && !next_event->error_flag && check_RN(next_event->sequence_number))
	{
		printf("[%f]CHECK_RN PASS for RN: %d\n", CURRENT_TIME, next_event->sequence_number);
		
		// Determine amount of shift window by (RN-P)
		window_sliding_amount = (next_event->sequence_number - P) % (N + 1);

		success_count += window_sliding_amount;

		// If modulo is negative
		if (window_sliding_amount < 0)
		{
			window_sliding_amount = window_sliding_amount + (N + 1);
		}
		printf("[%f]WINDOW SLIDING AMOUNT: %d\n", CURRENT_TIME, window_sliding_amount);

		// Slide the buffer by window_sliding_amount
		window_slide(window_sliding_amount);

		// Add the required new packets to the buffer after slide
		ptr_to_first_new_frame = add_new_packets(next_event->sequence_number - P);

		if (ptr_to_first_new_frame != NULL)
		{
			process_result = 1;
		}

		// Update the value of P
		P = next_event->sequence_number;
		printf("NEW VALUE OF P: %d\n", P);

		// Remove existing timeouts
		purge_timeout(timeout_ptr);

		// Register new timeout T[1])
		register_event('t', buffer_first->current_time + delta_timeout, -1, -1);
		printf("REGISTED NEW TIMEOUT AT: %f\n", buffer_first->current_time + delta_timeout);
		
		printf("ES AFTER ADDING NEW TIMEOUT\n");
		list_delete_event(first);

		print_list(first);
	}

	else if (next_event->type == 'a' && (next_event->error_flag || !check_RN(next_event->sequence_number)))
	{
		process_result = 2;
	}

	return process_result;
}

void sender()
{
	// P = SN[1] = 0
	P = buffer_first->sequence_number;

	frame_t packet_received;
	struct event *next_event;
	int process_result = 0;

	//Initialize current time to 0
	CURRENT_TIME = 0.0;

	while (success_count < DURATION)
	{
		// Transmit sequentially the N Packets in buffer
		struct gbn_frame *gbn_frame = buffer_first;

		while (gbn_frame->index <= N)
		{
			if (gbn_frame->index == 1)
				TIME_OF_FIRST_FRAME_IN_BUFFER = CURRENT_TIME + transmission_delay;
			else
				TIME_OF_FIRST_FRAME_IN_BUFFER += transmission_delay;
			
			gbn_frame->current_time = TIME_OF_FIRST_FRAME_IN_BUFFER;
			
			printf("\n");
			printf("Frame #: %d SENT AT %fs\n", gbn_frame->count, gbn_frame->current_time);
			print_item(gbn_frame);

			printf("-------- PACKET SENT --------\n");
			packet_received = send(gbn_frame);
			printf("-------- PACKET RECEIVED --------\n");

			if (!packet_received.is_null)
			{
				printf("Registered an ACK Event AT %f\n", CURRENT_TIME);
				register_event('a', packet_received.val, packet_received.error_flag, packet_received.sequence_number);
			}

			// After sending packet i, read the ES 
			// Check if an event (ACK OR TIMEOUT) occured
			next_event = read_es();

			// Condition 1
			// if (next_event == NULL) 
			// {
			// 	printf("CONDITION 1\n");
				
			// 	gbn_frame = gbn_frame->next;
			// 	continue;
			// }

			// Condition 2
			if (next_event->val < gbn_frame->current_time)
			{
				printf("CONDITION 2\n");

				process_result = process_next_event(next_event);
				printf("GOT PROCESS RESULT: %d\n", process_result);

				if (process_result == 0)
				{
					// Retransmit
					printf("Retranmitting ALL\n");
					printf("CURRENT BUFFER STATE:\n");
					print_buffer(buffer_first);
					
					gbn_frame = buffer_first;
					if (!gbn_frame)
						break;

					continue;
				}

				if (process_result == 1)
				{
					// Have a new frame added to the buffer
					// Schedule to transmit it right away
					// Pointer to it is located in ptr_to_first_new_frame
					gbn_frame = ptr_to_first_new_frame;
					if (!gbn_frame)
						break;

					continue;
				}

				if (process_result == 2)
				{
					gbn_frame = gbn_frame->next;
					if (!gbn_frame)
						break;

					continue;
				}
			} 
			else if (gbn_frame->index == N)
			{
				printf("CONDITION 3\n");
				struct event *ptr = first;
				while (ptr != NULL)
				{
					process_result = process_next_event(next_event);
					printf("GOT PROCESS RESULT: %d\n", process_result);

					if (process_result == 0)
					{
						// Retransmit
						printf("Retranmitting ALL\n");
						printf("CURRENT BUFFER STATE:\n");
						print_buffer(buffer_first);
						
						gbn_frame = buffer_first;
						if (!gbn_frame)
							break;

						continue;
					}

					if (process_result == 1)
					{
						// Have a new frame added to the buffer
						// Schedule to transmit it right away
						// Pointer to it is located in ptr_to_first_new_frame

						gbn_frame = ptr_to_first_new_frame;
						print_item(gbn_frame);

						if (!gbn_frame)
							break;

						break;
					}

					if (process_result == 2)
					{
						gbn_frame = gbn_frame->next;
						if (!gbn_frame)
							break;

						continue;
					}
					ptr = ptr->next;
				}
			} else
			{
				gbn_frame = gbn_frame->next;
				if (!gbn_frame)
					break;

				continue;
			}
		}
	}
	throughput = ((double)success_count * ((double)l))/CURRENT_TIME;
}

frame_t send(struct gbn_frame * gbn_frame)
{
	CURRENT_TIME = gbn_frame->current_time;

	// Register a timeout at T[1] + delta
	purge_timeout(timeout_ptr);
	
	register_event('t', buffer_first->current_time + delta_timeout, -1, -1);
	printf("Registered a TIMEOUT at: %f\n", buffer_first->current_time + delta_timeout);

	frame_t frame_in_send;
	frame_t send_output;

	printf("INPUT TO F_CHANNEL:\tTime = %f\tSN = %d\tL = %d\n", CURRENT_TIME, gbn_frame->sequence_number, frame_length);
	frame_in_send = channel(gbn_frame->sequence_number, frame_length);
	
	// Check for NULL Frame
	if (frame_in_send.is_null)
	{
		send_output.is_null = 1;
		return send_output;
	}

	printf("INPUT TO RECEIVER:\tTime = %f\tSN = %d\tERROR = %d\tIS_NULL = %d\n", CURRENT_TIME, frame_in_send.sequence_number, frame_in_send.error_flag, frame_in_send.is_null);
	frame_in_send = receiver(frame_in_send.sequence_number, frame_in_send.error_flag);
	
	printf("INPUT TO R_CHANNEL:\tTime = %f\tRN = %d\tH = %d\n", CURRENT_TIME, frame_in_send.sequence_number, H);
	frame_in_send = channel(frame_in_send.sequence_number, H);

	// Check for NULL ACK
	if (frame_in_send.is_null)
	{
		send_output.is_null = 1;
		return send_output;
	}

	printf("INPUT TO SENDER:\tTime = %f\tRN = %d\tERROR = %d\tIS_NULL = %d\n", CURRENT_TIME, frame_in_send.sequence_number, frame_in_send.error_flag, frame_in_send.is_null);

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
		rand_num = gen_rand(ber);
		if (rand_num == 0)
			zero_count++;
	}

	if (zero_count == 0)
	{
		// Correct frame
		channel_output.val = CURRENT_TIME + tau;
		CURRENT_TIME = channel_output.val;

		channel_output.error_flag = 0;
		channel_output.sequence_number = sequence_number;
		channel_output.is_null = 0;
		channel_output.size = -1;

	}
	else if (zero_count >= 1 && zero_count <= 4)
	{
		// Frame/ACK Error
		channel_output.val = CURRENT_TIME + tau;
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
	ack.val =  CURRENT_TIME + (double)H/(double)C;
	CURRENT_TIME = ack.val;

	return ack;
}



// double do_gbn() 
// {
// 	success_t success;
// 	success.num_success_packets = 0;

// 	int success_count = 0;
// 	double throughput = 0.0;

// 	fill_buffer();

// 	// P = SN[1] = 0
// 	CURRENT_TIME = 0.0;

// 	while (success_count < params.duration)
// 	{
// 		printf("SN: %d\n", SN);
// 		P = buffer_first->sequence_number;

// 		printf("###Buffer Before Transmitting:\n");
// 		print_buffer(buffer_first);

// 		list_cleanup(first);
		
// 		success = do_send();

// 		if (success.is_success)
// 		{
// 			printf("[%f]GOT SUCCESS\n", CURRENT_TIME);
// 			success_count += success.num_success_packets;
// 			printf("[%f]SUCCESS_COUNT: %d\n", CURRENT_TIME, success_count);
// 			CURRENT_TIME += first->val;
// 		}
// 	}

// 	printf("CURRENT_TIME: %f\n", CURRENT_TIME);
// 	throughput = ((double)success_count * ((double)params.packet_len))/CURRENT_TIME;
// 	return throughput;
// }

// success_t do_send()
// {
// 	success_t success; 
// 	frame_t packet_received;

// 	struct gbn_frame *gbn_frame = buffer_first;

// 	struct event *next_event;
// 	struct event *ptr = first;

// 	int window_sliding_amount = 0;


// 	while (gbn_frame->index <= params.N)
// 	{
// 		if (gbn_frame->index == 1)
// 			TIME_OF_FIRST_FRAME_IN_BUFFER = CURRENT_TIME + transmission_delay;
// 		else 
// 			TIME_OF_FIRST_FRAME_IN_BUFFER = TIME_OF_FIRST_FRAME_IN_BUFFER + transmission_delay;

// 		gbn_frame->current_time = TIME_OF_FIRST_FRAME_IN_BUFFER;

// 		printf("\n[%f]Frame #: %d\n", gbn_frame->current_time, gbn_frame->index);
// 		print_item(gbn_frame);
// 		printf("-------- SENDING NOW --------\n");

// 		// Send the packet to the channel with frame->current_time holding the value of CURRENT_TIME
// 		packet_received = send(gbn_frame);
// 		printf("-------- PACKET RECEIVED --------\n");

// 		if (!packet_received.is_null)
// 		{
// 			printf("[%f]Registered an ACK Event\n", CURRENT_TIME);
// 			register_event('a', packet_received.val, packet_received.error_flag, packet_received.sequence_number);
// 		}

// 		next_event = read_es();

// 		TIME_OF_FIRST_ACK = next_event->val;

// 		// Condition 1: No event occured, null event returned
// 		if (next_event == NULL) 
// 		{
// 			printf("NEXT EVENT IS NULL\n");
			
// 			gbn_frame = gbn_frame->next;
// 			if (!gbn_frame)
// 				break;

// 			continue;
// 		}
// 		// Condition 3: Current sent packet is last in the buffer
// 		else if (gbn_frame->index == params.N)
// 		{
// 			printf("GOT LAST FRAME IN WINDOW\n");
// 			struct event *ptr = first;
			
// 			while (ptr != NULL)
// 			{
// 				next_event = ptr;

// 				if (next_event->type == 't')
// 				{	
// 					retransmit_all();
// 				}
// 				else if (next_event->type == 'a')
// 				{
// 					if (next_event->error_flag == 0 && check_RN(next_event->sequence_number))
// 					{
// 						printf("[%f]CORRECT_RN for RN: %d\n", CURRENT_TIME, next_event->sequence_number);
// 						printf("[%f]PACKET RECEIVED WAS GOOD\n", CURRENT_TIME);
						
// 						// Determine amount of shift window by (RN-P)
// 						window_sliding_amount = (next_event->sequence_number - P) % (params.N + 1);

// 						if (window_sliding_amount < 0)
// 						{
// 							window_sliding_amount = window_sliding_amount + (params.N + 1);
// 						}
// 						printf("[%f]WINDOW SLIDING AMOUNT: %d\n", CURRENT_TIME, window_sliding_amount);


// 						// Shift the window
// 						dequeue_n(window_sliding_amount);

// 						printf("[%f]BUFFER AFTER SLIDE:\n", CURRENT_TIME);
// 						print_buffer(buffer_first);

// 						P = next_event->sequence_number;
// 						printf("[%f]NEW VALUE OF P: %d\n", CURRENT_TIME, P);

// 						// Remove existing timeouts
// 						purge_timeout(timeout_ptr);

// 						// Register new timeout T[1])
// 						register_event('t', buffer_first->current_time + params.delta_timeout, -1, -1);
						
// 						printf("REGISTED NEW TIMEOUT AT: %f\n", buffer_first->current_time + params.delta_timeout);
// 						print_list(first);

// 						list_delete_event(first);

// 						success.is_success = 1;
// 						success.num_success_packets += window_sliding_amount;

// 					}
// 					else if (next_event->error_flag == 1 || !check_RN(next_event->sequence_number))
// 					{
// 						printf("[%f]PACKET RECEIVED IN ERROR\n", CURRENT_TIME);
// 						// IGNORE
// 						// Continue sending the next packet
// 					}
// 				}	
// 				ptr = ptr->next;
// 			}
// 			return success;
// 		}
// 		// Condition 2: The WINDOW Slide 
// 		else 
// 		{
// 			if (next_event->val < gbn_frame->current_time)
// 			{
// 				printf("NEXT_EVENT TIME: %f <= GBN_FRAME TIME: %f\n", next_event->val, gbn_frame->current_time);

// 				// Condition 2: ACK or TIMEOUT
// 				printf("[%f]NEXT EVENT TYPE: %c\n", CURRENT_TIME, next_event->type);
				
// 				if (next_event->type == 't')
// 				{	
// 					retransmit_all();
// 				}
// 				else if (next_event->type == 'a')
// 				{
// 					if (next_event->error_flag == 0 && check_RN(next_event->sequence_number))
// 					{
// 						printf("[%f]CORRECT_RN for RN: %d\n", CURRENT_TIME, packet_received.sequence_number);
// 						printf("[%f]PACKET RECEIVED WAS GOOD\n", CURRENT_TIME);
						
// 						// Determine amount of shift window by (RN-P)
// 						window_sliding_amount = (packet_received.sequence_number - P) % (params.N + 1);

// 						if (window_sliding_amount < 0)
// 						{
// 							window_sliding_amount = window_sliding_amount + (params.N + 1);
// 						}
// 						printf("[%f]WINDOW SLIDING AMOUNT: %d\n", CURRENT_TIME, window_sliding_amount);

// 						// Shift the window
// 						dequeue_n(window_sliding_amount);

// 						printf("[%f]BUFFER AFTER SLIDE:\n", CURRENT_TIME);
// 						print_buffer(buffer_first);

// 						P = packet_received.sequence_number;
// 						printf("[%f]NEW VALUE OF P: %d\n", CURRENT_TIME, P);

// 						// Remove existing timeouts
// 						purge_timeout(timeout_ptr);

// 						// Register new timeout T[1])
// 						register_event('t', buffer_first->current_time + params.delta_timeout, -1, -1);
						
// 						printf("REGISTED NEW TIMEOUT AT: %f\n", buffer_first->current_time + params.delta_timeout);
// 						print_list(first);

// 						success.is_success = 1;
// 						success.num_success_packets += window_sliding_amount;
// 					}
// 					else if (next_event->error_flag == 1 || !check_RN(next_event->sequence_number))
// 					{
// 						printf("[%f]PACKET RECEIVED IN ERROR\n", CURRENT_TIME);
// 						// IGNORE
// 						// Continue sending the next packet
// 					}
// 					gbn_frame = gbn_frame->next;
// 					if (!gbn_frame)
// 						break;
// 				}	
// 			}
// 			else
// 			{
// 				printf("TRANSMITTING NEXT FRAME\n");
// 				gbn_frame = gbn_frame->next;
// 				if (!gbn_frame)
// 					break;

// 				continue;
// 			}	
// 		}
// 	}
// 	return success;
// }

int main(int argc, char *argv[]) 
{
	srand(time(0));	


	tau = 0.005;
	ber = 0;
	delta_timeout = 2.5*tau;

	fill_buffer();
	sender();
	printf("Throughput:\t%f\n", throughput);

}
