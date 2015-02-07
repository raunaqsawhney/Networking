#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>

#include "des.h"

// DES Linked List
struct event *first = (struct event *) NULL;
struct event *last = (struct event *) NULL;

struct result to_return;

float uniform()
{

	return ((float)rand()/(float)(RAND_MAX + 1.0));
}

float exponential(const float lambda)
{

  	float exp_value;
  	exp_value = (float)((-1.0 / lambda) * log(1.0 - uniform()));
	return exp_value;
}

void print_event(struct event *ptr)
{

	printf("Type:\t%c\n", ptr->type);
	printf("Time:\t%f\n", ptr->val);
	printf("Len :\t%f\n\n", ptr->len);
}

void print_list(struct event *ptr)
{
	while (ptr != NULL)
	{
		print_event(ptr);
		ptr = ptr->next;
	}
}

struct event * init_event(char type, float val, int packet_len)
{

	struct event *ptr;
	ptr = (struct event *)malloc(sizeof(struct event));
	if (ptr == NULL)
	{
		return (struct event *) NULL;
	}
	else {

		ptr->type = type;
		ptr->val = val;
		if (type == 'A')
		{
			ptr->len = exponential((1.0/(float)packet_len));
		}
		else
		{
			ptr->len = packet_len;
		}
		return ptr;
	}
	free(ptr);
}

void add(struct event *new)
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

void insert_event(struct event *new, struct event *from)
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

void delete_event(struct event *ptr)
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

void gen_observers(int alpha, int duration)
{
 
 	float current_time;
 	float tmp_time;
	struct event *ptr;

	while (current_time <= duration) {
		current_time += exponential(alpha);
		
		if (current_time > duration) {
			break;
		}

		ptr = init_event('O', current_time, 0);
		add(ptr);
	}

}

void gen_arrivals(float lambda, int duration, int packet_len)
{
 	
 	float current_time = 0;
	struct event *ptr;
	struct event *prev = first;

	while (current_time <= duration) {
		current_time += exponential(lambda);
		
		if (current_time > duration) {
			break;
		}

		ptr = init_event('A', current_time, packet_len);
		insert_event(ptr, prev);
		prev = ptr;
	}
}

void cleanup( struct event *ptr )
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

void run_system(int link_rate, int buffer_size)
{

	clock_t g_start, g_end;

	int num_packet_arrivals = 0; 				// N_a = number of packet arrivals so far
	int num_packet_departures = 0;  			// N_d = number of packet departures so far
	int num_observations = 0;					// N_o = number of observations so far
	long double num_packets_in_system = 0;		// N_p = number of packets in the system (N_a - N_d)
	int num_idle = 0;							// N_i = idle counter
	int num_dropped = 0;						// N_d = dropped counter
	int num_generated = 0;						// N_gen = packet generated counter

	float arrival_time = 0.0;
	float service_time = 0.0;
	float departure_time = 0.0;

	struct event *ptr = first;
	struct event *new_departure; 
	struct event *temp;

	int buffer_count = 0;

	struct event *buffer[buffer_size];
	struct event *highest_time = first;

	while (ptr != NULL)
	{
		if (ptr->type == 'A')
		{
			num_generated = num_generated + 1;

			if (num_packet_arrivals - num_packet_departures <= buffer_size || buffer_size == 0)
			{
				arrival_time = ptr->val;
				service_time = (ptr->len/(float)link_rate);

				if (num_packet_arrivals - num_packet_departures == 0)
				{
					// Queue empty, service right away
					departure_time = arrival_time + service_time;
					
					#ifdef DEBUG
						printf("Queue EMPTY, no need for buffer, departure time: %f\n", departure_time);
					#endif
				}
				else
				{
					// Queue not empty, add arrival packet to buffer
					departure_time = departure_time + service_time;
					
					#ifdef DEBUG
						printf("Queue NOT EMPTY, MAY need buffer, departure time: %f\n", departure_time);
					#endif
				}

				new_departure = init_event('D', departure_time, 0);
				insert_event(new_departure, highest_time);
				
				if (new_departure->val >= highest_time->val)
				{
					highest_time = new_departure;
				}

				num_packet_arrivals = num_packet_arrivals + 1;

			}
			else
			{
				num_dropped = num_dropped + 1;
			}
		}
		else if (ptr->type == 'D')
		{
			num_packet_departures = num_packet_departures + 1;
			
			// if (buffer_count > 0 && buffer_size > 0)
			// {
			// 	#ifdef DEBUG
			// 		printf("Removing from buffer\n");
			// 	#endif

			// 	buffer[buffer_count] = NULL;
			// 	buffer_count = buffer_count - 1;
			// }
		}
		else if (ptr->type == 'O')
		{
			num_observations = num_observations + 1;
			
			num_packets_in_system = num_packets_in_system + (num_packet_arrivals - num_packet_departures);
			if (num_packet_arrivals - num_packet_departures == 0)
			{
				num_idle = num_idle + 1;
			}
		}
		ptr = ptr->next;
	}
	compute_metrics(num_packets_in_system, num_observations, num_packet_arrivals, num_packet_departures, num_idle, num_dropped, num_generated);
}

void compute_metrics(long double num_packets_in_system, int num_observations, int num_packet_arrivals, int num_packet_departures, int num_idle, int num_dropped, int num_generated)
{

	printf("\n");

	printf("****** SIMULATOR METRICS ******\n");
	printf("Observations: %14d\n", num_observations);
	printf("Arrivals: %18d\n", num_packet_arrivals);
	printf("Departures: %16d\n", num_packet_departures);
	printf("\n");
	printf("N(t): %22Lf\n", num_packets_in_system);
	printf("E[N]: %22.5f\n", (float)num_packets_in_system/num_observations);
	printf("Idle Count: %16d\n", num_idle);
	printf("P_idle: %20.5f\n", (float)num_idle/num_observations);
	printf("Dropped: %19d\n", num_dropped);
	printf("P_loss: %20.5f\n", (float)num_dropped/num_generated);
	printf("*******************************\n");


	to_return.num_observations = num_observations;
	to_return.num_packet_arrivals = num_packet_arrivals;
	to_return.num_packet_departures = num_packet_departures;
	to_return.num_packets_in_system = num_packets_in_system;
	to_return.avg_packets_in_system = (float)num_packets_in_system/num_observations;
	to_return.num_idle = num_idle;
	to_return.probability_idle = (float)num_idle/num_observations;
	to_return.num_dropped = num_dropped;
	to_return.probability_dropped = (float)num_dropped/num_generated;

	printf("\n");
}

struct result simulator(int T, float r, int L, int C, int k) {

	clock_t g_start, g_end;
	g_start = clock();
	srand(time(0));

	#ifdef DEBUG
		printf("DEBUGGING ENABLED\n");
	#endif

	printf("\n");

	// Initialize variables
	// params: T, roh, L, C, alpha, B
	int duration = T; 		// T in seconds
	float roh = r;	  			// roh: utilization of queue (L * (lambda/C)))
	int packet_len = L;	// L: average length of a packet in bits
	int link_rate = C;		// C: Transmission rate of output link in bits/second
	int buffer_size = k;	// k: Buffer size
	double total_time;

	if (buffer_size < 0)
	{
		buffer_size = 0;
	}

	float lambda = (roh / packet_len) * link_rate; 
	float alpha = lambda + 10;
	
	// Print helper text
	printf("********* PARAMETERS **********\n");
	printf("Duration: %10d seconds\n", duration);
	printf("Alpha: %21.5f\n", alpha); 
	printf("Lambda: %20.5f\n", lambda);
	printf("Packet Length: %8d bits\n", packet_len);
	printf("Link Rate: %13d bps\n", link_rate);
	printf("Buffer Size: %15d\n", buffer_size);
	printf("Roh: %23.5f\n", roh);
	printf("*******************************\n");

	to_return.duration = duration;
	to_return.alpha = alpha;
	to_return.lambda = lambda;
	to_return.packet_len = packet_len;
	to_return.link_rate = link_rate;
	to_return.buffer_size = buffer_size;
	to_return.roh = roh;

	// Run the simulator
	gen_observers(alpha, duration);
	gen_arrivals(lambda, duration, packet_len);
	run_system(link_rate, buffer_size);
	
	cleanup(first);

	// Stop timing measurement
	g_end = clock();
	total_time = ((double) (g_end-g_start))/ CLOCKS_PER_SEC;
	to_return.total_time = total_time;

	printf("TOTAL TIME: %f seconds\n", total_time);
	printf("\n");
	printf("-------------------------------\n");

	return to_return;
}
