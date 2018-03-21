
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "dbrew.h"
#include <sys/time.h>


#define RES (10)
#define ITER (100)
#define rewrite_getter (0)
#define rewrite_apply (1)
#define variants (0)
#define reordered (1)
#define foo (0)
typedef double (*apply_fun) (int, int, double**, int);
typedef double* (*get_func) (int, int, double**, int);

inline double* get(int, int, double**, int);
double apply(int, int, double**, int);

void swap (double**, double**, int);
void print(double**, int);



int main(int argc, char **argv)
{
	struct timeval start, end;

	int i, j, iter = 0, verbose = 0;
	apply_fun af, af1, af2, af3, af4;
	Rewriter* r, * r2, *r3, *r4, *r5 = 0;
	RewriterConfig* rc;
	af1 = apply;
	double** m0, ** m1;
	double* m0_0,* m0_1,* m1_0, * m1_1;

	int runs = 0, size = 0, rewrite = 0, reorder = 0, variant = 0;
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

	if (!size)
		size = 10;
	if (!iter)
		iter = 5;
	if (!runs)
		runs = 1;


	//size = RES;
	// allocate both matrices
	//if (size % 2)
	//	size++;
	m0_0 = (double*) malloc (sizeof(double)*size*size/2);
	m0_1 = (double*) malloc(sizeof(double) * size * (size + size % 2) / 2);
	m0 = (double**) malloc (sizeof(double*)*2);
	m0 [0] = m0_0;
	m0 [1] = m0_1;

	m1_0 = (double*) malloc (sizeof(double)*size*size/2);
	m1_1 = (double*) malloc(sizeof(double) * size * (size + size % 2) / 2);
	m1 = (double**) malloc (sizeof(double*)*2);
	m1 [0] = m1_0;
	m1 [1] = m1_1;
	assert(m0_0 && m0_1 && m1_0 && m1_1 && m0 && m1);

	af = apply;


	if (rewrite) {
		fprintf(stderr, "Rewriting apply\n");
		r2 = dbrew_new();
		if (variant) {
			r3 = dbrew_new();
			r4 = dbrew_new();
			r5 = dbrew_new();
		}
		rc = config_new();
		dbrew_rewriter_set_config(r2, rc);
		if (variant) {
			dbrew_rewriter_set_config(r3, rc);
			dbrew_rewriter_set_config(r4, rc);
			dbrew_rewriter_set_config(r5, rc);


			dbrew_set_function(r3, (uint64_t) apply);
			dbrew_set_function(r4, (uint64_t) apply);
			dbrew_set_function(r5, (uint64_t) apply);
		}

		dbrew_set_function(r2, (uint64_t) apply);


		dbrew_config_parcount(rc, 4);

		//dbrew_config_staticpar(rc, 3); // size is constant

		dbrew_range_set(r2, 3, size, size); // size is always 10. also sets cs to CS_STATIC2
		dbrew_range_set(r2, 0, 1, size - 2); // x is between 1 and size-2
		// testing purposes
		//dbrew_range_set(r2, 0, 1, 1);
		dbrew_range_set(r2, 1, 1, size - 2); // y is between 1 and size-2


		if (variant) {
			dbrew_config_rangepar(rc, 0); // for specific variants of function
		}
		if (verbose > 1) {
			dbrew_verbose(r2, true, true, true);
			dbrew_optverbose(r2, true);
			dbrew_config_function_setname(rc, (uint64_t) apply, "Apply");
		}
		//af = (apply_fun) dbrew_rewrite(r2, 1, 1, m0, size);
		if (rewrite) {
			af1 = (apply_fun) dbrew_rewrite(r2, 1, 1, m0, size);
		}
		if (variants) {
			af2 = (apply_fun) dbrew_rewrite(r3, (size / 2) - 1, 1, m0, size);
			af3 = (apply_fun) dbrew_rewrite(r4, (size) / 2, 1, m0, size);
			af4 = (apply_fun) dbrew_rewrite(r5, size - 1, 1, m0, size);
		}
	}
	if (r2 && (verbose > 0)) {
		// use another rewriter to show generated code
		Rewriter* rr = dbrew_new();
		RewriterConfig* rc2 = config_new();
		dbrew_rewriter_set_config(rr, rc2);

		uint64_t genfunc = dbrew_generated_code(r2);
		int gensize = dbrew_generated_size(r2);
		//dbrew_config_function_setname(r2, genfunc, "gen");
		dbrew_config_function_setsize(rc2, genfunc, gensize);
		dbrew_decode_print(rr, genfunc, gensize);
		dbrew_free(rr);
		config_free(rc2);
	}

	double res;
	for (int k = 0; k < runs; ++k) {
		// initialize both matrices
		for (j = 0; j < size; ++j)
			for (i = 0; i < size; ++i) {
				*(get(i, j, m0, size)) = 0.0;
				*(get(i, j, m1, size)) = 0.0;
			}
		// initialize upper and lower row
		for (i = 0; i < size; ++i) {
			*(get(i, 0, m0, size)) = 1.0;
			*(get(i, size - 1, m0, size)) = 1.5;
			*(get(0, i, m0, size)) = 2.0;
			*(get(size - 1, i, m0, size)) = 2.5;


			*(get(i, 0, m1, size)) = 1.0;
			*(get(i, size - 1, m1, size)) = 1.5;
			*(get(0, i, m1, size)) = 2.0;
			*(get(size - 1, i, m1, size)) = 2.5;
		}
		//print(m0, size);
		//print(get, m1);

		// run update function
		gettimeofday(&start, 0);
		for (int c = 0; c < iter; ++c) {
			if (reorder) {
				for (j = 1; j < size - 1; ++j)

					for (i = 1; i < size / 2 - 1; ++i)
						*get(i, j, m1, size) = af1(i, j, m0, size);
				for (j = 1; j < size - 1; ++j)
					*get(size / 2 - 1, j, m1, size) = af1(size / 2 - 1, j, m0,
							size);
				for (j = 1; j < size - 1; ++j)
					*get(size / 2, j, m1, size) = af1(size / 2, j, m0, size);
				for (i = size / 2 + 1; i < size - 1; ++i)
					for (j = 1; j < size - 1; ++j)
						*get(i, j, m1, size) = af1(i, j, m0, size);

			} else {
				if (rewrite) {
					if (variant) {
						for (j = 1; j < size - 1; ++j)

							for (i = 1; i < size / 2 - 1; ++i)
								*get(i, j, m1, size) = af1(i, j, m0, size);
						for (j = 1; j < size - 1; ++j)
							*get(size / 2 - 1, j, m1, size) = af2(size / 2 - 1,
									j, m0, size);
						for (j = 1; j < size - 1; ++j)
							*get(size / 2, j, m1, size) = af3(size / 2, j, m0,
									size);
						for (i = size / 2 + 1; i < size - 1; ++i)
							for (j = 1; j < size - 1; ++j)
								*get(i, j, m1, size) = af4(i, j, m0, size);
					} else {
						for (j = 1; j + 1 < size; ++j) {
							for (i = 1; i < size - 1; ++i)
								*get(i, j, m1, size) = af1(i, j, m0, size);
						}
					}
				} else {
					for (j = 1; j + 1 < size; ++j) {
					for (i = 1; i + 1 < size; ++i) {
						//uf(i, j, m0, m1, size);
						*get(i, j, m1, size) = af(i, j, m0, size);
						}
					}
				}
			}
			swap(m0, m1, 2);
			//print(get, m1);

		}
		//print(m0, size);
		gettimeofday(&end, 0);
		res += (double) (end.tv_usec - start.tv_usec) / 1000000
				+ (double) end.tv_sec - start.tv_sec;
	}

	res = res / runs;
	/*
	fprintf(stderr, "\n\nSize: %d\n\n", size);
	if (rewrite)
		fprintf(stderr, "Rewritten function\n");
	if (reorder)
		fprintf(stderr, "Reordered loops\n");
	if (variant)
		fprintf(stderr, "Specialized for variants\n");

	fprintf(stderr, "\nAvg over %d runs of %d iterations: %.2fs\n\n", runs,
			iter, res);
	 */
	// free allocated heap memory
	if (m0_0)
		free(m0_0);
	if (m0_1)
		free(m0_1);
	if (m1_0)
		free(m1_0);
	if (m1_1)
		free(m1_1);
	if (m0)
		free(m0);
	if (m1)
		free(m1);
	if (r)
		dbrew_free(r);
	if (r2)
		dbrew_free(r2);
	if (rc)
		config_free(rc);

	return 0;
}

inline
double* get(int x, int y, double** matrices, int size) {
	// determine the matrix to which the point belongs
	if (x < (size / 2)/*(size/2)*/) {
		// left matrix, row-major
		return &(matrices[0][x + y * size / 2]);
	} else {
		// right matrix, column
		return &(matrices[1][y + (x - size / 2) * size]);
	}
	return 0;
}
void swap(double** m1, double** m2, int size) {
	int i;
	double* temp;
	for (i = 0; i < size; ++i) {
		temp = m1[i];
		m1[i] = m2[i];
		m2[i] = temp;
	}

}

double apply(int x, int y, double** m, int size) {
	double retval = (*get(x, y - 1, m, size) + *get(x - 1, y, m, size)
			+ *get(x + 1, y, m, size) + *get(x, y + 1, m, size)) / 4;
	return retval;

}

void print(double** m1, int size) {
	int i, j;
	fprintf(stderr, "\n");
	for (j = 0; j < size; ++j)
	{
		for (i = 0; i < size; ++i)
		{
			fprintf(stderr, " %f ", *get(i, j, m1, size));
		}
		fprintf(stderr, "\n");
	}
	fprintf(stderr, "\n");
}
