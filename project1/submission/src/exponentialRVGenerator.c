#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

float uniform() {

	return ((float)rand()/(float)(RAND_MAX + 1.0));
}

float exponential(const float lambda) {
  	float exp_value;

  	exp_value = (float)((-1.0 / lambda) * log(1 - uniform()));

	return exp_value;
}

int main(int argc, char *argv[]) {
	float e, lambda;
	int num_vars;

	num_vars = atoi(argv[1]);
	lambda = atof(argv[2]);

	float array[num_vars];

	int i;
	srand(time(0));

	float mean, variance;
	float sum = 0;
	float sum1 = 0;

	for (i = 0; i < num_vars; i++) {
		e = exponential(lambda);
		printf("%.5f\n",e );

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

	printf("RAW MEAN:\t\t%f\n", raw_mean);
	printf("RAW VARIANCE:\t\t%f\n\n", raw_variance);

	printf("Excepted MEAN: \t\t%f\n", expected_mean);
	printf("Expected VARIANCE: \t%f\n", expected_variance);

	printf("\n");
	printf("MEAN DIFFERENCE: \t%.5f %%\n", fabs((raw_mean-expected_mean)/((raw_mean+expected_mean)/2))*100);
	printf("VARIANCE DIFFERENCE: \t%.5f %%\n\n", fabs((raw_variance-expected_variance)/((raw_variance+expected_variance)/2))*100);

}

