#include "valrange.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "instr.h"

static int64_t power(int64_t base, int exp)
{
	int64_t ret = 1;
	for (int i = 0; i< exp; ++i)
	{
		ret*=base;
	}
	return ret;
}
ValRangePool* valrange_allocPool(int s)
{
	ValRangePool* p;
	p = (ValRangePool*) malloc(
			sizeof(ValRangePool) + s * sizeof(ValRange)
					+ s * sizeof(ValRangeTuple));
    p->size = s;
    p->used = 0;
	return p;
}


void valrange_freePool(ValRangePool* p)
{
	free (p);
}

void valrange_update_range(ValRangeTuple *valRange, int64_t min, int64_t max)
{
	if (valRange->min<min)
	{
		valRange->min = min;
	}
	if (valRange->max>max)
	{
		valRange->max=max;
	}
	
}


ValRange* valrange_new(ValRangePool* p)
{
    ValRange* valRange;
    assert(p && (p->used < p->size));

    valRange = p->valRange + p->used;
    valRange->p = p;
	valRange->vt = VT_None;
	valRange->size = 8;
	valRange->used = 0;
    p->used++;

	return valRange;
}
ValRangeTuple* valrange_newtuple(ValRange* vr, int64_t min, int64_t max) {
	ValRangeTuple* vrt;
	assert(vr && (vr->used < vr->size));
	vrt = &(vr->vrt[vr->used]);
	vr->used++;
	vrt->min = min;
	vrt->max = max;
	return vrt;
}


ValRange* valrange_new_const(ValRangePool* p, int64_t value, ValType vt)
{
	ValRange* valRange = valrange_new(p);
	valrange_newtuple(valRange, value, value);
	valRange->vt = vt;
	return valRange;
}

ValRange* valrange_new_init(ValRangePool* p, int64_t minVal, int64_t maxVal,
		ValType vt)
{
	ValRange* valRange = valrange_new(p);
	valrange_newtuple(valRange, minVal, maxVal);

	valRange->vt = vt;
	return valRange;
}


ValRange* valrange_copy(ValRangePool* p, ValRange* vr_old)
{
	if (!vr_old)
		return 0;
	assert(p);
	ValRange* valRange = valrange_new(p);
	for (int i = 0; i < vr_old->used; ++i) {
		valrange_newtuple(valRange, vr_old->vrt[i].min,
				vr_old->vrt[i].max);


	}
	valRange->vt = vr_old->vt;
	return valRange;
}

ValRange* valrange_add(ValRangePool* p, ValRange* vr, int value)
{
	if (!vr)
		return 0;
	ValRange* ret = valrange_copy(p, vr);
	for (int i = 0; i < ret->used; ++i) {
		ret->vrt[i].min = (ret->vrt[i].min + value);
		ret->vrt[i].max = (ret->vrt[i].max + value);
	}
	return ret;
}
ValRange* valrange_scale(ValRangePool* p, ValRange* vr, int value)
{
	
	if (!vr)
		return 0;

	ValRange* ret = valrange_copy(p, vr);
	for (int i = 0; i < ret->used; ++i) {
		ret->vrt[i].min *= value;
		ret->vrt[i].max *= value;
	}
	return ret;
}
ValRange* valrange_divide(ValRangePool* p, ValRange* vr, int value)
{
	if (!vr)
		return 0;
	ValRange* ret = valrange_copy(p, vr);
	for (int i = 0; i < ret->used; ++i) {
		ret->vrt[i].min /= value;
		ret->vrt[i].max /= value;
	}
	return ret;
}
ValRange* valrange_sar(ValRangePool* p, ValRange* vr, int value)
{
	if (!vr)
		return 0;
	bool doret = true;
	ValRange* ret = valrange_copy(p, vr);
	for (int i = 0; i < ret->used; ++i) {
		ret->vrt[i].min = ret->vrt[i].min / power(2, value);
		ret->vrt[i].max = ret->vrt[i].max / power(2, value);
		if (ret->vrt[i].min > ret->vrt[i].max)
			doret = false;
	}
	return doret ? ret : 0;
}

ValRange* valrange_shr(ValRangePool* p, ValRange* vr, int value)
{

	if (!vr)
		return 0;

	ValRange* ret = valrange_copy(p, vr);
	for (int i = 0; i < ret->used; ++i) {

		uint32_t min = ret->vrt[i].min, max = ret->vrt[i].max;
		min = min >> (uint32_t) value;
		max = max >> (uint32_t) value;

		/*

	ret->min = ret->min >> value;
	ret->max = ret->max >> value;
		 */
		ret->vrt[i].min = min;
		ret->vrt[i].max = max;
		if (ret->vrt[i].min > ret->vrt[i].max) {
			uint32_t tmp = ret->vrt[i].min;
			ret->vrt[i].min = ret->vrt[i].max;
			ret->vrt[i].max = tmp;
			//return 0;
		}
	}
	return ret;
}

ValRange* valrange_sum(ValRangePool* p, ValRange* vr, ValRange* vr2)
{

	if (!(vr && vr2))
		return 0;

	ValRange* ret = valrange_copy(p, vr);
	assert(ret->size >= (vr->used * vr2->used));
	for (int i = 0; i < vr->used; ++i) {
		for (int j = 0; j < vr2->used; ++j) {
			{
				ret->vrt[i * vr2->used + j].min = vr->vrt[i].min
						+ vr2->vrt[j].min;
				ret->vrt[i * vr2->used + j].max = vr->vrt[i].max
						+ vr2->vrt[j].max;

			}
		}
	}
	return ret;

}

ValRange* valrange_product(ValRangePool* p, ValRange* vr, ValRange* vr2)
{

	if (!(vr && vr2))
		return 0;

	ValRange* ret = valrange_copy(p, vr);
	assert(ret->size >= (vr->used * vr2->used));
	for (int i = 0; i < vr->used; ++i) {
		for (int j = 0; j < vr2->used; ++j) {
			int64_t values[4];
			values[0] = vr->vrt[i].min * vr2->vrt[j].min;
			values[1] = vr->vrt[i].min * vr2->vrt[j].max;
			values[2] = vr->vrt[i].max * vr2->vrt[j].min;
			values[3] = vr->vrt[i].max * vr2->vrt[j].max;
			int64_t min = values[0], max = values[0];
			for (int r = 1; r < 4; ++r) {
				if (min > values[r])
					min = values[r];
				if (max < values[r])
					max = values[r];
			}
			ret->vrt[i * vr2->used + j].min = min;
			ret->vrt[i * vr2->used + j].max = max;


		}
	}
	return ret;

}
ValRange* valrange_diff(ValRangePool* p, ValRange* vr, ValRange* vr2)
{

	if (!(vr && vr2))
		return 0;
	ValRange* ret = valrange_copy(p, vr);
	assert(ret->size >= (vr->used * vr2->used));
	for (int i = 0; i < vr->used; ++i) {
		for (int j = 0; j < vr2->used; ++j) {
			{
				ret->vrt[i * vr2->used + j].min = vr->vrt[i].min
						- vr2->vrt[j].min;
				ret->vrt[i * vr2->used + j].max = vr->vrt[i].max
						- vr2->vrt[j].max;

			}
		}
	}
	return ret;

}

void valrange_free(ValRange* valRange)
{
	if (valRange)
		free (valRange);
}
/*
void valrange_setmin (ValRange* vr, int val)
{
	if (vr->min<val)
		vr->min = val;
}
void valrange_setmax (ValRange* vr, int val)
{
	if (vr->max>val)
		vr->max = val;
}
 */

static
int appendValRange(char* b, ValRange* vr)
{
	int off = 0;
	if (!vr)
		return off;
	for (int i = 0; i < vr->used; ++i) {
		if (vr->vrt[i].min != vr->vrt[i].max)
		{
			off += sprintf(b + off, "Min ");
			off += sprintf(b + off, "%ld", vr->vrt[i].min);
			off += sprintf(b + off, ", Max ");
			off += sprintf(b + off, "%ld ", vr->vrt[i].max);
		} else {
			off += sprintf(b + off, "Const val ");
			off += sprintf(b + off, "%ld ", vr->vrt[i].min);
		}
	}
	if (!(vr->vt == VT_8 || vr->vt == VT_32 || vr->vt == VT_64))
		off += sprintf(b + off, "XX");
	return off;
}

bool isConst(ValRange* vr)
{
	if (vr==0) return false;
	int value = vr->vrt[0].min;
	for (int i = 0; i < vr->used; ++i) {
		if (vr->vrt[i].min == vr->vrt[i].max) {
			if (vr->vrt[i].min != value) {
				return false;
			}
		} else
			return false;
	}
	return true;
}
static
bool isInValRangeTuple(int val, ValRangeTuple* vrt) {
	return ((val >= vrt->min) && val <= vrt->max);

}

bool isInValRange(int val, ValRange* vr) {
	for (int i = 0; i < vr->used; ++i) {
		if (isInValRangeTuple(val, &(vr->vrt[i])))
			return true;
	}
	return false;
}


char *valrange_toString(ValRange* vr)
{
    static char buf[200];
    int off;
	off = appendValRange(buf, vr);
    assert(off < 200);
    return buf;
}

bool vrIsEqual(ValRange* vr1, ValRange* vr2) {
	if (!(vr1 || vr2))
		return true;
	if (!(vr1 && vr2))
		return false;
	if (vr1->used != vr2->used)
		return false;
	if (vr1->vt != vr2->vt)
		return false;
	if (!(vr1 && vr2))
		return true;
	if (!(vr1 || vr2))
		return false;
	for (int i = 0; i < vr1->used; ++i) {

		if (!(vr1->vrt[i].min == vr2->vrt[i].min))
		return false;
		if (!(vr1->vrt[i].max == vr2->vrt[i].max))
		return false;
	}
	return true;
}



