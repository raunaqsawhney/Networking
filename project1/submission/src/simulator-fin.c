#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "des.c"

int test_expo(int);
int sim_inf_buffer(int);
int sim_fin_buffer(int);

FILE *fp;

int test_exponential(int num_iterations_test_expo) 
{
	srand(time(0));
	int num_vars = 1000;
	int lambda = 75;

	int j;
	for (j = 0; j < num_iterations_test_expo; j++)
	{
		float e;
		float array[num_vars];

		int i;

		float mean, variance;
		float sum = 0;
		float sum1 = 0;

		for (i = 0; i < num_vars; i++) {
			e = exponential(lambda);
			sum += e;
			array[i] = e;
		}

		int size_of_array = sizeof(array)/sizeof(array[0]);
		mean = sum / num_vars;

		for (i = 0; i < size_of_array; i++) {
			sum1 += pow(array[i]-mean, 2);
		}

		double raw_mean = mean;
		double raw_variance = sum1 / (num_vars - 1);

		double expected_mean = (1.0)/lambda;
		double expected_variance = (1.0)/pow(lambda, 2);

		double mean_difference = fabs((raw_mean-expected_mean)/((raw_mean+expected_mean)/2))*100;
		double variance_difference = fabs((raw_variance-expected_variance)/((raw_variance+expected_variance)/2))*100;

		printf("[%d] Raw Mean:\t%f | Expected Mean:\t%f | Mean Difference:\t%f %% | Raw Variance:\t%f | Expected Variance:\t%f | Variance Difference:\t%f %%\n", j, raw_mean, expected_mean, mean_difference, raw_variance, expected_variance, variance_difference);
	}

	return 1;
}

int sim_inf_buffer(num_iterations_sim_inf)
{
	struct result got_return;
	fp = fopen("sim_inf_buffer.csv", "a+");

	int packet_len = 12000;
	int link_rate = 1000000;
	int duration = 10000;

	float roh;
	//Question 3
	fprintf(fp, "Question 3:\tInfinite Buffer\n");
	fprintf(fp, "T,alpha,lambda,L,C,k,Roh,T_taken,N_o,N_a,N_d,N(t),E[N],N_idle,P_idle,N_loss,P_loss\n");

	for (roh = 0.25+0.1; roh < 1.5; roh += 0.1)
	{
		got_return = simulator(duration, roh, packet_len, link_rate, -1);
		fprintf(fp, "%d,%f,%f,%d,%d,%d,%f,%f,%d,%d,%d,%Lf,%.5f,%d,%.5f,%d,%.5f\n", got_return.duration, got_return.alpha, got_return.lambda, got_return.packet_len, got_return.link_rate, got_return.buffer_size, got_return.roh, got_return.total_time, got_return.num_observations, got_return.num_packet_arrivals, got_return.num_packet_departures, got_return.num_packets_in_system, got_return.avg_packets_in_system, got_return.num_idle, got_return.probability_idle, got_return.num_dropped, got_return.probability_dropped);
	}

	fprintf(fp, "\n");

	// Question 4
	roh = 1.2;
	got_return = simulator(duration, roh, packet_len, link_rate, -1);
	fprintf(fp, "%d,%f,%f,%d,%d,%d,%f,%f,%d,%d,%d,%Lf,%.5f,%d,%.5f,%d,%.5f\n", got_return.duration, got_return.alpha, got_return.lambda, got_return.packet_len, got_return.link_rate, got_return.buffer_size, got_return.roh, got_return.total_time, got_return.num_observations, got_return.num_packet_arrivals, got_return.num_packet_departures, got_return.num_packets_in_system, got_return.avg_packets_in_system, got_return.num_idle, got_return.probability_idle, got_return.num_dropped, got_return.probability_dropped);
	fprintf(fp, "\n\n");
	fclose(fp);

	return 1;
}

int sim_fin_buffer(num_iterations_sim_fin)
{
	struct result got_return;

	int packet_len = 12000;
	int link_rate = 1000000;
	int duration = 11000;
	fp = fopen("sim_fin_buffer.csv", "a+");

	float roh;
	// Question 6.1
	fprintf(fp, "\n");
	fprintf(fp, "Question 6.1:\tFinite Buffer\n");
	fprintf(fp, "T,alpha,lambda,L,C,k,Roh,T_taken,N_o,N_a,N_d,N(t),E[N],N_idle,P_idle,N_loss,P_loss\n");

	for (roh = 0.5+0.1; roh < 1.5; roh += 0.1)
	{
		got_return = simulator(duration, roh, packet_len, link_rate, 5);
		fprintf(fp, "%d,%f,%f,%d,%d,%d,%f,%f,%d,%d,%d,%Lf,%.5f,%d,%.5f,%d,%.5f\n", got_return.duration, got_return.alpha, got_return.lambda, got_return.packet_len, got_return.link_rate, got_return.buffer_size, got_return.roh, got_return.total_time, got_return.num_observations, got_return.num_packet_arrivals, got_return.num_packet_departures, got_return.num_packets_in_system, got_return.avg_packets_in_system, got_return.num_idle, got_return.probability_idle, got_return.num_dropped, got_return.probability_dropped);

		got_return = simulator(duration, roh, packet_len, link_rate, 10);
		fprintf(fp, "%d,%f,%f,%d,%d,%d,%f,%f,%d,%d,%d,%Lf,%.5f,%d,%.5f,%d,%.5f\n", got_return.duration, got_return.alpha, got_return.lambda, got_return.packet_len, got_return.link_rate, got_return.buffer_size, got_return.roh, got_return.total_time, got_return.num_observations, got_return.num_packet_arrivals, got_return.num_packet_departures, got_return.num_packets_in_system, got_return.avg_packets_in_system, got_return.num_idle, got_return.probability_idle, got_return.num_dropped, got_return.probability_dropped);
		
		got_return = simulator(duration, roh, packet_len, link_rate, 40);
		fprintf(fp, "%d,%f,%f,%d,%d,%d,%f,%f,%d,%d,%d,%Lf,%.5f,%d,%.5f,%d,%.5f\n", got_return.duration, got_return.alpha, got_return.lambda, got_return.packet_len, got_return.link_rate, got_return.buffer_size, got_return.roh, got_return.total_time, got_return.num_observations, got_return.num_packet_arrivals, got_return.num_packet_departures, got_return.num_packets_in_system, got_return.avg_packets_in_system, got_return.num_idle, got_return.probability_idle, got_return.num_dropped, got_return.probability_dropped);

	}

	//Question 6.2.1
	fprintf(fp, "\n");
	fprintf(fp, "Question 6.2.1:\tFinite Buffer\n");
	fprintf(fp, "T,alpha,lambda,L,C,k,Roh,T_taken,N_o,N_a,N_d,N(t),E[N],N_idle,P_idle,N_loss,P_loss\n");

	for (roh = 0.4+0.1; roh <= 2.0; roh += 0.1)
	{
		got_return = simulator(duration, roh, packet_len, link_rate, 5);
		fprintf(fp, "%d,%f,%f,%d,%d,%d,%f,%f,%d,%d,%d,%Lf,%.5f,%d,%.5f,%d,%.5f\n", got_return.duration, got_return.alpha, got_return.lambda, got_return.packet_len, got_return.link_rate, got_return.buffer_size, got_return.roh, got_return.total_time, got_return.num_observations, got_return.num_packet_arrivals, got_return.num_packet_departures, got_return.num_packets_in_system, got_return.avg_packets_in_system, got_return.num_idle, got_return.probability_idle, got_return.num_dropped, got_return.probability_dropped);

		got_return = simulator(duration, roh, packet_len, link_rate, 10);
		fprintf(fp, "%d,%f,%f,%d,%d,%d,%f,%f,%d,%d,%d,%Lf,%.5f,%d,%.5f,%d,%.5f\n", got_return.duration, got_return.alpha, got_return.lambda, got_return.packet_len, got_return.link_rate, got_return.buffer_size, got_return.roh, got_return.total_time, got_return.num_observations, got_return.num_packet_arrivals, got_return.num_packet_departures, got_return.num_packets_in_system, got_return.avg_packets_in_system, got_return.num_idle, got_return.probability_idle, got_return.num_dropped, got_return.probability_dropped);

		got_return = simulator(duration, roh, packet_len, link_rate, 40);
		fprintf(fp, "%d,%f,%f,%d,%d,%d,%f,%f,%d,%d,%d,%Lf,%.5f,%d,%.5f,%d,%.5f\n", got_return.duration, got_return.alpha, got_return.lambda, got_return.packet_len, got_return.link_rate, got_return.buffer_size, got_return.roh, got_return.total_time, got_return.num_observations, got_return.num_packet_arrivals, got_return.num_packet_departures, got_return.num_packets_in_system, got_return.avg_packets_in_system, got_return.num_idle, got_return.probability_idle, got_return.num_dropped, got_return.probability_dropped);

	}

	got_return = simulator(duration, 2.0, packet_len, link_rate, 5);
	fprintf(fp, "%d,%f,%f,%d,%d,%d,%f,%f,%d,%d,%d,%Lf,%.5f,%d,%.5f,%d,%.5f\n", got_return.duration, got_return.alpha, got_return.lambda, got_return.packet_len, got_return.link_rate, got_return.buffer_size, got_return.roh, got_return.total_time, got_return.num_observations, got_return.num_packet_arrivals, got_return.num_packet_departures, got_return.num_packets_in_system, got_return.avg_packets_in_system, got_return.num_idle, got_return.probability_idle, got_return.num_dropped, got_return.probability_dropped);

	got_return = simulator(duration, 2.0, packet_len, link_rate, 10);
	fprintf(fp, "%d,%f,%f,%d,%d,%d,%f,%f,%d,%d,%d,%Lf,%.5f,%d,%.5f,%d,%.5f\n", got_return.duration, got_return.alpha, got_return.lambda, got_return.packet_len, got_return.link_rate, got_return.buffer_size, got_return.roh, got_return.total_time, got_return.num_observations, got_return.num_packet_arrivals, got_return.num_packet_departures, got_return.num_packets_in_system, got_return.avg_packets_in_system, got_return.num_idle, got_return.probability_idle, got_return.num_dropped, got_return.probability_dropped);

	got_return = simulator(duration, 2.0, packet_len, link_rate, 40);
	fprintf(fp, "%d,%f,%f,%d,%d,%d,%f,%f,%d,%d,%d,%Lf,%.5f,%d,%.5f,%d,%.5f\n", got_return.duration, got_return.alpha, got_return.lambda, got_return.packet_len, got_return.link_rate, got_return.buffer_size, got_return.roh, got_return.total_time, got_return.num_observations, got_return.num_packet_arrivals, got_return.num_packet_departures, got_return.num_packets_in_system, got_return.avg_packets_in_system, got_return.num_idle, got_return.probability_idle, got_return.num_dropped, got_return.probability_dropped);


	//Question 6.2.2
	fprintf(fp, "\n");
	fprintf(fp, "Question 6.2.2:\tFinite Buffer\n");
	fprintf(fp, "T,alpha,lambda,L,C,k,Roh,T_taken,N_o,N_a,N_d,N(t),E[N],N_idle,P_idle,N_loss,P_loss\n");

	for (roh = 2.0+0.2; roh <= 5.0; roh += 0.2)
	{
		got_return = simulator(duration, roh, packet_len, link_rate, 5);
		fprintf(fp, "%d,%f,%f,%d,%d,%d,%f,%f,%d,%d,%d,%Lf,%.5f,%d,%.5f,%d,%.5f\n", got_return.duration, got_return.alpha, got_return.lambda, got_return.packet_len, got_return.link_rate, got_return.buffer_size, got_return.roh, got_return.total_time, got_return.num_observations, got_return.num_packet_arrivals, got_return.num_packet_departures, got_return.num_packets_in_system, got_return.avg_packets_in_system, got_return.num_idle, got_return.probability_idle, got_return.num_dropped, got_return.probability_dropped);

		got_return = simulator(duration, roh, packet_len, link_rate, 10);
		fprintf(fp, "%d,%f,%f,%d,%d,%d,%f,%f,%d,%d,%d,%Lf,%.5f,%d,%.5f,%d,%.5f\n", got_return.duration, got_return.alpha, got_return.lambda, got_return.packet_len, got_return.link_rate, got_return.buffer_size, got_return.roh, got_return.total_time, got_return.num_observations, got_return.num_packet_arrivals, got_return.num_packet_departures, got_return.num_packets_in_system, got_return.avg_packets_in_system, got_return.num_idle, got_return.probability_idle, got_return.num_dropped, got_return.probability_dropped);

		got_return = simulator(duration, roh, packet_len, link_rate, 40);
		fprintf(fp, "%d,%f,%f,%d,%d,%d,%f,%f,%d,%d,%d,%Lf,%.5f,%d,%.5f,%d,%.5f\n", got_return.duration, got_return.alpha, got_return.lambda, got_return.packet_len, got_return.link_rate, got_return.buffer_size, got_return.roh, got_return.total_time, got_return.num_observations, got_return.num_packet_arrivals, got_return.num_packet_departures, got_return.num_packets_in_system, got_return.avg_packets_in_system, got_return.num_idle, got_return.probability_idle, got_return.num_dropped, got_return.probability_dropped);

	}
	// Question 6.2.3
	fprintf(fp, "\n");
	fprintf(fp, "Question 6.2.3:\tFinite Buffer\n");
	fprintf(fp, "T,alpha,lambda,L,C,k,Roh,T_taken,N_o,N_a,N_d,N(t),E[N],N_idle,P_idle,N_loss,P_loss\n");

	for (roh = 5.0+0.4; roh < 10.0; roh += 0.4)
	{
		got_return = simulator(duration, roh, packet_len, link_rate, 5);
		fprintf(fp, "%d,%f,%f,%d,%d,%d,%f,%f,%d,%d,%d,%Lf,%.5f,%d,%.5f,%d,%.5f\n", got_return.duration, got_return.alpha, got_return.lambda, got_return.packet_len, got_return.link_rate, got_return.buffer_size, got_return.roh, got_return.total_time, got_return.num_observations, got_return.num_packet_arrivals, got_return.num_packet_departures, got_return.num_packets_in_system, got_return.avg_packets_in_system, got_return.num_idle, got_return.probability_idle, got_return.num_dropped, got_return.probability_dropped);

		got_return = simulator(duration, roh, packet_len, link_rate, 10);
		fprintf(fp, "%d,%f,%f,%d,%d,%d,%f,%f,%d,%d,%d,%Lf,%.5f,%d,%.5f,%d,%.5f\n", got_return.duration, got_return.alpha, got_return.lambda, got_return.packet_len, got_return.link_rate, got_return.buffer_size, got_return.roh, got_return.total_time, got_return.num_observations, got_return.num_packet_arrivals, got_return.num_packet_departures, got_return.num_packets_in_system, got_return.avg_packets_in_system, got_return.num_idle, got_return.probability_idle, got_return.num_dropped, got_return.probability_dropped);

		got_return = simulator(duration, roh, packet_len, link_rate, 40);
		fprintf(fp, "%d,%f,%f,%d,%d,%d,%f,%f,%d,%d,%d,%Lf,%.5f,%d,%.5f,%d,%.5f\n", got_return.duration, got_return.alpha, got_return.lambda, got_return.packet_len, got_return.link_rate, got_return.buffer_size, got_return.roh, got_return.total_time, got_return.num_observations, got_return.num_packet_arrivals, got_return.num_packet_departures, got_return.num_packets_in_system, got_return.avg_packets_in_system, got_return.num_idle, got_return.probability_idle, got_return.num_dropped, got_return.probability_dropped);

	}
	return 1;

	// fprintf(fp, "\n\n");
	fclose(fp);
}

int main(int argc, char *argv[])
{
	clock_t g_start, g_end;

	printf("\n");
	printf("======== ECE 358 Lab 1 ========\n");
	printf("Submitted By: Raunaq Sawhney\n");
	printf("UW ID: rsawhney\n");
	printf("Student ID: 20421357\n");
	printf("===============================\n");

	int num_iterations_test_expo = 3;
	int num_iterations_sim_inf = 1;
	int num_iterations_sim_fin = 1;

	g_start = clock();

	int test_expo = 1;//test_exponential(num_iterations_test_expo);
	int sim_inf = 1;//sim_inf_buffer(num_iterations_sim_inf);
	int sim_fin = sim_fin_buffer(num_iterations_sim_fin);

	g_end = clock();

	if (test_expo == 1 && sim_inf == 1 && sim_fin == 1)
	{
		printf("Simulation Complete in %f. seconds\n", ((double) (g_end-g_start))/ CLOCKS_PER_SEC);
	}

}
