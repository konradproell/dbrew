#ifndef valrange_h
#define valrange_h

#include <stdint.h>
#include <stdbool.h>
#include "instr.h"

typedef struct _ValRange ValRange;
typedef struct _ValRangePool ValRangePool;
typedef struct _ValRangeTuple ValRangeTuple;

struct _ValRangeTuple {
	int64_t min;
	int64_t max;
};

struct _ValRange {
	/*
	 int64_t min;
	 int64_t max;
	 */
	int size;
	int used;
	ValType vt;
	ValRangePool* p;
	ValRangeTuple vrt[8];
};

struct _ValRangePool {
	int size;
	int used;
	ValRange valRange[1];
};

ValRangePool* valrange_allocPool(int size);

void valrange_freePool(ValRangePool*);

void valrange_setmin (ValRange*, int);
void valrange_setmax (ValRange*, int);


void valrange_update_range(ValRangeTuple *valRange, int64_t min, int64_t max);

void vr_addtuple(ValRange* vr, int64_t min, int64_t max);
ValRange* valrange_add(ValRangePool* p, ValRange* vr, int value);

ValRange* valrange_sum(ValRangePool* p, ValRange* vr, ValRange* vr2);
ValRange* valrange_diff(ValRangePool* p, ValRange* vr, ValRange* vr2);

ValRange* valrange_scale(ValRangePool* p, ValRange* vr, int value);

ValRange* valrange_divide(ValRangePool* p, ValRange* vr, int value);
ValRange* valrange_product(ValRangePool* p, ValRange* vr, ValRange* vr2);

ValRange* valrange_copy (ValRangePool* p, ValRange *valRange);

ValRange* valrange_new_const(ValRangePool* p, int64_t value, ValType vt);

ValRange* valrange_sar(ValRangePool* p, ValRange* vr, int value);
ValRange* valrange_shr(ValRangePool* p, ValRange* vr, int value);




void valrange_free(ValRange* valRange);

bool isInValRange(int, ValRange*);

ValRangeTuple* valrange_newtuple(ValRange* vr, int64_t min, int64_t max);
ValRange* valrange_new(ValRangePool* p);

ValRange* valrange_new_init(ValRangePool* p, int64_t minVal, int64_t maxVal,
		ValType vt);

bool isConst(ValRange*);

char *valrange_toString(ValRange* vr);

bool vrIsEqual(ValRange* vr1, ValRange* vr2);

#endif /* valrange_h */
