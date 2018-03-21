/**
 * This file is part of DBrew, the dynamic binary rewriting library.
 *
 * (c) 2015-2016, Josef Weidendorfer <josef.weidendorfer@gmx.de>
 *
 * DBrew is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License (LGPL)
 * as published by the Free Software Foundation, either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * DBrew is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with DBrew.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "expr.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

ExprPool* expr_allocPool(int s)
{
    ExprPool* p;
    p = (ExprPool*) malloc(sizeof(ExprPool) + s * sizeof(ExprNode));
    p->size = s;
    p->used = 0;
    return p;
}

void expr_freePool(ExprPool* p)
{
    free(p);
}

ExprNode* expr_newNode(ExprPool* p, NodeType t)
{
    ExprNode* e;

    assert(p && (p->used < p->size));
    e = p->n + p->used;
    e->p = p;
    e->type = t;
	e->left = e->right = 0;
    p->used++;
    return e;
}

int expr_nodeIndex(ExprPool* p, ExprNode* n)
{
    assert(p && n && n->p == p);
    return (int)(n - p->n);
}

static ExprNode* getNodeByIndex(ExprPool* p, int index) {
	return &(p->n[index]);
}

ExprNode* expr_newConst(ExprPool* p, int val)
{
    ExprNode* e = expr_newNode(p, NT_Const);
    e->ival = val;

    return e;
}

ExprNode* expr_newPar(ExprPool* p, int no, char *n)
{
    ExprNode* e = expr_newNode(p, NT_Par);
    e->ival = no;
    if (n) {
        int i = 0;
        while(n[i] && (i < EN_NAMELEN-1)) {
            e->name[i] = n[i];
            i++;
        }
        e->name[i] = 0;
    }
    else
        e->name[0] = 0;

    return e;
}

ExprNode* expr_newScaled(ExprPool* p, int factor, ExprNode* exp)
{
	if ((factor == -1 && exp->type==NT_Scaled &&exp->ival==-1))
	{
		return &(exp->p->n[exp->left]);
	}
	else if	(factor == -1 && exp->type==NT_Const &&exp->ival==-1)
	{
		return expr_newConst(exp->p, 1);
	}
	else
	{
    ExprNode* e = expr_newNode(p, NT_Scaled);
    e->ival = factor;
    e->left = expr_nodeIndex(p, exp);

    return e;
	}
}



ExprNode* expr_newRef(ExprPool* p, uint64_t ptr, char* n, ExprNode* idx)
{
    ExprNode* e = expr_newNode(p, NT_Ref);
    e->ptr = ptr;
    e->left = expr_nodeIndex(p, idx);
    if (n) {
        int i = 0;
        while(n[i] && (i < EN_NAMELEN-1)) {
            e->name[i] = n[i];
            i++;
        }
        e->name[i] = 0;
    }
    else
        e->name[0] = 0;

    return e;
}

ExprNode* expr_newSum(ExprPool* p, ExprNode* left, ExprNode* right)
{
    ExprNode* e = expr_newNode(p, NT_Sum);
    e->left = expr_nodeIndex(p, left);
    e->right = expr_nodeIndex(p, right);

    return e;
}
ExprNode* expr_newProduct(ExprPool* p, ExprNode* left, ExprNode* right)
{
    ExprNode* e = expr_newNode(p, NT_Product);
    e->left = expr_nodeIndex(p, left);
    e->right = expr_nodeIndex(p, right);

    return e;
}

ExprNode* expr_newDiv(ExprPool* p, ExprNode* left, ExprNode* right)
{
    ExprNode* e = expr_newNode(p, NT_Div);
    e->left = expr_nodeIndex(p, left);
    e->right = expr_nodeIndex(p, right);

    return e;
}
ExprNode* expr_newShiftleft(ExprPool* p, ExprNode* left, ExprNode* right)
{
    ExprNode* e = expr_newNode(p, NT_Shiftleft);
    e->left = expr_nodeIndex(p, left);
    e->right = expr_nodeIndex(p, right);

    return e;
}

ExprNode* expr_newShiftright(ExprPool* p, ExprNode* left, ExprNode* right)
{
    ExprNode* e = expr_newNode(p, NT_Shiftright);
    e->left = expr_nodeIndex(p, left);
    e->right = expr_nodeIndex(p, right);

    return e;
}

ExprNode* expr_newCompare(ExprPool* p, ExprNode* left, ExprNode* right)
{
    ExprNode* e = expr_newNode(p, NT_Cmp);
    e->left = expr_nodeIndex(p, left);
    e->right = expr_nodeIndex(p, right);

    return e;
}

static
int appendExpr(char* b, ExprNode* e)
{
    int off = 0;
    switch(e->type) {
    case NT_Const:
        return sprintf(b, "%d", e->ival);

    case NT_Par:
        if (e->name[0])
            return sprintf(b, "%s", e->name);
        return sprintf(b, "par%d", e->ival);

    case NT_Ref:
        if (e->name[0])
            off = sprintf(b, "%s", e->name);
        else
            off = sprintf(b, "%lx", e->ptr);
        b[off++] = '[';
        off += appendExpr(b+off, e->p->n + e->left);
        off += sprintf(b+off, "]");
        return off;

    case NT_Scaled:
        off =  sprintf(b, "%d * ", e->ival);
        off += appendExpr(b+off, e->p->n + e->left);
        return off;

    case NT_Sum:
        off =  appendExpr(b, e->p->n + e->left);
        off += sprintf(b+off, " + ");
        off += appendExpr(b+off, e->p->n + e->right);
        return off;
    case NT_Product:
        off =  appendExpr(b, e->p->n + e->left);
        off += sprintf(b+off, " * ");
        off += appendExpr(b+off, e->p->n + e->right);
        return off;
    case NT_Div:
        off =  appendExpr(b, e->p->n + e->left);
        off += sprintf(b+off, " / ");
        off += appendExpr(b+off, e->p->n + e->right);
        return off;
    case NT_Shiftleft:
        off =  appendExpr(b, e->p->n + e->left);
        off += sprintf(b+off, " << ");
        off += appendExpr(b+off, e->p->n + e->right);
        return off;
    case NT_Shiftright:
        off =  appendExpr(b, e->p->n + e->left);
        off += sprintf(b+off, " >> ");
        off += appendExpr(b+off, e->p->n + e->right);
        return off;
    case NT_Cmp:
    	off = sprintf(b, "CMP (");
        off +=  appendExpr(b+off, e->p->n + e->left);
        off += sprintf(b+off, ", ");
        off += appendExpr(b+off, e->p->n + e->right);
        off += sprintf(b+off, ")");

    	return off;
    default: assert(0);
    }
    return 0;
}

char *expr_toString(ExprNode *e)
{
    static char buf[200];
    int off;
    off = appendExpr(buf, e);
    assert(off < 200);
    return buf;
}

// returns how many NT_PAR nodes are in one expression
static
int numPars (ExprNode *expr)
{
	ExprNode* leftNode, *rightNode;
	leftNode = &(expr->p->n[expr->left]);
	rightNode = &(expr->p->n[expr->right]);


	switch (expr->type)
	{
	case NT_Par: return 1;
	case NT_Sum:
	case NT_Product:
	case NT_Cmp:
	case NT_Div:
	case NT_Shiftright:
	case NT_Shiftleft:
		return (numPars(leftNode)+numPars(rightNode));
	case NT_Scaled:
	case NT_Ref:
		return (numPars(leftNode));
	case NT_Const:
		return 0;
	default: assert (0);
	}

	return 0;
}

// returns whether there is exactly one NT_PAR node in expression
bool onePar(ExprNode* expr)
{
	return (numPars(expr)==1);
}

// takes cmp expr. returns cmp expr, where one side is just par and other is expression
ExprNode* solve_expr(ExprNode* expr)
{
	if (!onePar(expr)) {
		fprintf(stderr, "Error. Can not solve expression %s.\n",
				expr_toString(expr));
		assert(0); //we only can evaluate expressions with one unknown

		return 0;
	}
	ExprNode *ret, *par, *eval, *tmp;
	if (expr->type==NT_Cmp)
	{
		// asign nodes
		if (onePar(&(expr->p->n[expr->left])))
		{
			par = &(expr->p->n[expr->left]);
			eval = &(expr->p->n[expr->right]);
		}
		else
		{
			eval = &(expr->p->n[expr->left]);
			par = &(expr->p->n[expr->right]);
		}
		// other side has to be static
		if (!(eval->type==NT_Const))
		{
			return 0;
		}
		while (1)
		{
			if (par->type==NT_Par)
			{
				break;
			}
			switch (par->type)
			{
			case NT_Sum:
				if (onePar(&(expr->p->n[par->left])))
				{
					tmp = &(expr->p->n[par->right]);
					par = &(expr->p->n[par->left]);
				}
				else
				{
					tmp = &(expr->p->n[par->left]);
					par = &(expr->p->n[par->right]);
				}
				eval = expr_newSum(expr->p, eval, expr_newScaled(expr->p, -1, tmp));
				break;
			default: return 0;
			}
		}
		// root expr has to be cmp
		eval = eval_expr(eval);
		ret = expr_newCompare(expr->p, par, eval);
		assert(expr->p->n[ret->left].type==NT_Par);
		return ret;
	}
	assert(0);
	return 0;
}

static
bool calc_expr(ExprNode* expr, int* retval)
{
	int left=0, right=0;

	switch (expr->type)
	{
	case NT_Sum:
		if (calc_expr(&(expr->p->n[expr->left]), &left)&&
				calc_expr(&(expr->p->n[expr->right]), &right))
		{
			*retval = left + right;
			break;
		}
		else return false;
	case NT_Const:
		*retval = expr->ival;
		break;
	case NT_Scaled:
		if (calc_expr(&(expr->p->n[expr->left]), &left))
		{
			*retval = left * expr->ival;
			break;
		}
		else return false;
	default: return false;;
	}

	return true;
}

int parNum(ExprNode* expr)
{
	assert(expr->type==NT_Par);
	return expr->ival;
}

ExprNode* eval_expr(ExprNode* expr)
{
	ExprNode *ret = 0;
	if (numPars(expr)) return 0;
	int val;
	if (calc_expr(expr, &val))
		ret = expr_newConst(expr->p, val);
	return ret;
}

bool expr_isEqual(ExprNode* e1, ExprNode* e2)
{
	if (!(e1 || e2))
		return true;
	if (!(e1 && e2))
		return false;
	ExprNode *e11 = 0, *e12 = 0, *e21 = 0, *e22 = 0;
	if (e1->left)
		e11 = getNodeByIndex(e1->p, e1->left);
	if (e1->right)
		e12 = getNodeByIndex(e1->p, e1->right);
	if (e2->left)
		e21 = getNodeByIndex(e2->p, e2->left);
	if (e2->right)
		e22 = getNodeByIndex(e2->p, e2->right);

	if (e1->type!=e2->type)
		return false;
	switch (e1->type) {
	case NT_Const:
		return e1->ival == e2->ival;
	case NT_Par:
		return e1->ival == e2->ival;
	case NT_Ref:
		return ((e1->ptr == e2->ptr) && expr_isEqual(e1, e2));
	case NT_Sum:
		return ((expr_isEqual(e11, e21) && expr_isEqual(e12, e22))
				|| (expr_isEqual(e11, e22) && expr_isEqual(e12, e21)));
		break;
	default:
		break;
	}
	return true;
}
