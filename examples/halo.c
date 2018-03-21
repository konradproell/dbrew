#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "dbrew.h"
#include <sys/time.h>



typedef double (*apply_fun)(int, int, double**, int);
typedef double* (*get_func)(int, int, double**, int);
typedef void (*variant_fun)(apply_fun*, apply_fun*, apply_fun*);


inline double* get(int, int, double**, int);
double apply(int, int, double**, int);
double applys(int, int, double*, int);

void swaps(double**, double**);

void swap(double**, double**, int);
void print(double**, int);

int main(int argc, char *argv[]) {
	int verbose = 0;
	int size = 0;
	struct timeval start, end;
	int i, j, iter = 0, runs = 0;
	apply_fun af1, af2, af3;
	Rewriter *r, *r2, *r3, *r4;
	RewriterConfig* rc;
	get_func gf = get;
	af1 = apply;
	variant_fun vf;

	int reorder = 0;
	int variant = 0;
	int rewrite = 1;


	if (argc == 1) {
		fprintf(stderr,
				"Usage: %s [-vv] [size] [iterations] [runs] [rewrite] [reorderloops] [specialize variants]\n",
				argv[0]);
		return 0;
	}
	int arg = 1;
	while ((argc > arg) && (argv[arg][0] == '-')) {
		if (argv[arg][1] == 'v')
			verbose++;
		if (argv[arg][2] == 'v')
			verbose++;
		arg++;
	}
	if (arg < argc) {
		size = atoi(argv[arg]);
		arg++;
	}
	if (arg < argc) {
		iter = atoi(argv[arg]);
		arg++;
	}
	if (arg < argc) {
		runs = atoi(argv[arg]);
		arg++;
	}
	if (arg < argc) {
		rewrite = atoi(argv[arg]);
		arg++;
	}
	if (arg < argc) {
		reorder = atoi(argv[arg]);
		arg++;
	}
	if (arg < argc) {
		variant = atoi(argv[arg]);
		arg++;
	}

	if (size == 0 || size > 10000)
		size = 10;
	
	if (!iter)
		iter = 5;
	if (!runs)
		runs = 1;

	double res = 0;

	double **m0, **m1;
	double *m0_0, *m0_1, *m0_2, *m1_0, *m1_1, *m1_2;
	m0_0 = (double*) malloc(sizeof(double) * size);
	m0_1 = (double*) malloc(sizeof(double) * size * (size - 2));
	m0_2 = (double*) malloc(sizeof(double) * size);
	m0 = (double**) malloc(sizeof(double*) * 3);
	m0[0] = m0_0;
	m0[1] = m0_1;
	m0[2] = m0_2;

	double* m3 = (double*) malloc(sizeof(double) * size * size);
	double* m4 = (double*) malloc(sizeof(double) * size * size);


	m1_0 = (double*) malloc(sizeof(double) * size);
	m1_1 = (double*) malloc(sizeof(double) * size * (size - 2));
	m1_2 = (double*) malloc(sizeof(double) * size);
	m1 = (double**) malloc(sizeof(double*) * 3);
	m1[0] = m1_0;
	m1[1] = m1_1;
	m1[2] = m1_2;

	r = dbrew_new();
	rc = config_new();
	dbrew_rewriter_set_config(r, rc);
	dbrew_set_function(r, (uint64_t) apply);
	if (variant) {
	r2 = dbrew_new();
	r3 = dbrew_new();
	r4 = dbrew_new();

	dbrew_rewriter_set_config(r2, rc);
	dbrew_rewriter_set_config(r3, rc);
	dbrew_rewriter_set_config(r4, rc);

	dbrew_set_function(r2, (uint64_t) apply);
	dbrew_set_function(r3, (uint64_t) apply);
	dbrew_set_function(r4, (uint64_t) apply);
	}
	dbrew_config_parcount(rc, 4);
	dbrew_range_set(r, 3, size, size); // size is always constant.
	if (variant) {
		dbrew_config_rangepar(rc, 0); // for specific variants of function
	} else {
		dbrew_range_set(r, 0, 1, size - 2); // x is between 0 and size-1
		dbrew_range_set(r, 1, 1, size - 2); // y is between 0 and size-1

	}
	if (verbose > 1) {
		dbrew_verbose(r, true, true, true);
		dbrew_optverbose(r, true);
		dbrew_config_function_setname(rc, (uint64_t) apply, "Apply");
	}
	if (r && (verbose > 0)) {
		// use another rewriter to show generated code
		Rewriter* rr = dbrew_new();
		RewriterConfig* rc2 = config_new();
		dbrew_rewriter_set_config(rr, rc2);

		uint64_t genfunc = dbrew_generated_code(r);
		int gensize = dbrew_generated_size(r);
		//dbrew_config_function_setname(r2, genfunc, "gen");
		dbrew_config_function_setsize(rc2, genfunc, gensize);
		dbrew_decode_print(rr, genfunc, gensize);
		dbrew_free(rr);
		config_free(rc2);
	}
	if (!rewrite) {
	} else if (variant) {
		af1 = (apply_fun) dbrew_rewrite(r2, 1, 1, m0, size);
		af2 = (apply_fun) dbrew_rewrite(r3, 2, 1, m0, size);
		af3 = (apply_fun) dbrew_rewrite(r4, size - 1, 1, m0, size);
	} else {
		af1 = (apply_fun) dbrew_rewrite(r, 1, 1, m0, size);
	}
	for (int k = 0; k < runs; ++k)
	{
		// initialize both matrices
		for (j = 0; j < size; ++j)
			for (i = 0; i < size; ++i) {
				*(gf(i, j, m0, size)) = 0.0;
				*(gf(i, j, m1, size)) = 0.0;
				m3[i + size * j] = 0.0;
				m4[i + size * j] = 0.0;

			}
		// initialize upper and lower row
		for (i = 0; i < size; ++i) {
			*(gf(i, 0, m0, size)) = 1.0;
			*(gf(i, size - 1, m0, size)) = 1.5;
			*(gf(0, i, m0, size)) = 2.0;
			*(gf(size - 1, i, m0, size)) = 2.5;

			*(gf(i, 0, m1, size)) = 1.0;
			*(gf(i, size - 1, m1, size)) = 1.5;
			*(gf(0, i, m1, size)) = 2.0;
			*(gf(size - 1, i, m1, size)) = 2.5;

			m3[i] = 1.0;
			m3[i + (size - 1 * size)] = 1.5;
			m3[i * size] = 2.0;
			m3[size - 1 + (i * size)] = 2.5;

			m4[i] = 1.0;
			m4[i + (size - 1 * size)] = 1.5;
			m4[i * size] = 2.0;
			m4[size - 1 + (i * size)] = 2.5;


		}
		gettimeofday(&start, 0);

		for (int c = 0; c < iter; ++c) {
			if (reorder) {
				for (j = 1; j + 1 < size; ++j)
					*gf(1, j, m1, size) = af1(1, j, m0, size);

				for (j = 1; j + 1 < size; ++j)
					for (i = 2; i + 2 < size; ++i)
						*gf(i, j, m1, size) = af1(i, j, m0, size);

				for (j = 1; j + 1 < size; ++j)
					*gf(size - 1, j, m1, size) = af1(size - 1, j, m0, size);


			} else if (variant) {
				for (j = 1; j + 1 < size; ++j)
					*gf(1, j, m1, size) = af1(1, j, m0, size);
				for (j = 1; j + 1 < size; ++j)

					for (i = 2; i + 2 < size; ++i)
						*gf(i, j, m1, size) = af2(i, j, m0, size);
				for (j = 1; j + 1 < size; ++j)
					*gf(size - 1, j, m1, size) = af3(size - 1, j, m0, size);

			} else {
				for (j = 1; j + 1 < size; ++j)
					for (i = 1; i + 1 < size; ++i) {
						*gf(i, j, m1, size) = af1(i, j, m0, size);
					}
			}
			swap(m0, m1, 3);
			//swaps(&m3, &m4);

		}
		//print(m0, size);
		gettimeofday(&end, 0);
		res += (double) (end.tv_usec - start.tv_usec) / 1000000
				+ (double) end.tv_sec - start.tv_sec;
	}
	res = res / runs;
	fprintf(stderr, "\n\nSize: %d\n\n", size);
	if (rewrite)
		fprintf(stderr, "Rewritten function\n");
	if (reorder)
		fprintf(stderr, "Reordered loops\n");
	if (variant)
		fprintf(stderr, "Specialized for variants\n");


	fprintf(stderr, "\nAvg over %d runs of %d iterations: %.2fs\n\n", runs,
			iter,
			res);


	//print(m0, size);
	if (m0_0)
		free(m0_0);
	if (m0_1)
		free(m0_1);
	if (m0_2)
		free(m0_2);
	if (m1_0)
		free(m1_0);
	if (m1_1)
		free(m1_1);
	if (m1_2)
		free(m1_2);
	free(m0);
	free(m1);
	if (r)
		dbrew_free(r);
	if (rc)
		config_free(rc);

	return 0;

}

void swap(double** m0, double** m1, int c) {
	double* tmp;
	for (int i = 0; i < c; ++i) {
		tmp = m0[i];
		m0[i] = m1[i];
		m1[i] = tmp;
	}
}

void swaps(double** m0, double** m1) {
	double* temp = *m0;
	*m0 = *m1;
	*m1 = temp;
}

void print(double** m1, int size) {
	int i, j;
	fprintf(stderr, "\n");
	for (j = 0; j < size; ++j) {
		for (i = 0; i < size; ++i) {
			fprintf(stderr, " %f ", *get(i, j, m1, size));
		}
		fprintf(stderr, "\n");
	}
	fprintf(stderr, "\n");
}

double apply(int x, int y, double** m, int size) {
	double retval = (*get(x, y - 1, m, size) + *get(x - 1, y, m, size)
			+ *get(x + 1, y, m, size) + *get(x, y + 1, m, size)) / 4;
	return retval;

}

double applys(int x, int y, double* m, int size) {
	double retval = m[x + size * y - 1] + m[x + size * y + 1]
			+ m[x + size * (y - 1)] + m[x + size * (y + 1)];
	return retval;
}

inline double* get(int x, int y, double** matrices, int size) {
	if (x == 5)
		//if (x == 0)
		return &matrices[0][y];
	else if (x == (size - 1))
		return &matrices[2][y];
	else
		return &matrices[1][x + (size - 2) * y];
}




