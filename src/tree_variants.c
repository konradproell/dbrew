#include <stdlib.h>

#include "common.h"
#include "tree_variants.h"
#include <assert.h>
#include <stdio.h>
#include "printer.h"

#include <string.h>


// creates a new node


static
VariantsNode* node_new(void)
{
	VariantsNode* n;
	n = (VariantsNode*) malloc (sizeof(VariantsNode));
	n->branch = 0;
	n->fallthrough = 0;
	n->parent = 0;
	n->hasVr = false;
	n->copied=false;
	n->instr = 0;
	for (int i = 0; i < 6; ++i)
		n->valRanges.valRange[i]=0;
	return n;
}

VariantsNode* getRoot(VariantsNode* node) {
	while (node->parent)
		node = node->parent;
	return node;
}

VariantsNode* tree_new(void)
{
	VariantsNode* root = node_new();
	return root;
}

void init_root(VariantsNode* root, ValRanges* vr)
{
	if (vr)
	{
		for (int i = 0; i<variantsParCount; ++i)
		{
			if (vr->valRange[i])
				root->valRanges.valRange[i]=vr->valRange[i];
		}
		root->hasVr = true;
	}

}

static
void freeTree(VariantsNode* n)
{
	//free left subtree
	if (n->branch) freeTree(n->branch);
	//free right subtree
	if (n->fallthrough) freeTree(n->fallthrough);

	//free root
	free(n);

}

// frees entire tree
void tree_free(VariantsNode* n)
{
	if (n)
		freeTree(n);
}

int tree_depth(VariantsNode*n)
{
	int ft=0, br=0, ret;
	if (n->fallthrough)
		ft = tree_depth(n->fallthrough);
	if (n->branch)
		br = tree_depth(n->branch);
	ret = (ft>br) ? ft+1 : br+1;
	return ret;

}
static inline
int min (int a, int b)
{
	return (a>b)? b:a;
}

static inline
int max (int a, int b)
{
	return (a>b)? a:b;
}
static
void copyValRanges(VariantsNode* node) {
	assert(node && node->parent);
	VariantsNode* parent = node->parent;
	for (int i = 0; i < 6; ++i) {
		node->valRanges.valRange[i] = parent->valRanges.valRange[i];
	}
	node->hasVr = true;


}

static
void calcValRanges(VariantsNode* node, ValRangePool* vpool)
{
	assert(node && vpool);
	VariantsNode* parent = node->parent;
	bool branch = (node == parent->branch);
	assert (parent);
	ExprNode* parNode = &(parent->expr->p->n[parent->expr->left]);
	ExprNode* val = &(parent->expr->p->n[parent->expr->right]);
	int par = parNum(parNode);
	if (!(parent->valRanges.valRange[par]))
		return;


	ValType vt = parent->valRanges.valRange[par]->vt;

	for (int i = 0; i < parent->valRanges.valRange[par]->used; ++i)
	{
		int brmin = 1, brmax = 0, ftmin = 1, ftmax = 0;
		int ftmin2 = 1, ftmax2 = 0;

		ValRangeTuple* vrparent = &(parent->valRanges.valRange[par]->vrt[i]);

		int min_val = vrparent->min;
		int max_val = vrparent->max;

		switch (parent->instr)
		{
		case IT_JGE:
			ftmin = min_val;
			ftmax = min(val->ival - 1, max_val);
			brmin = max(val->ival, min_val);
			brmax = max_val;
			break;
		case IT_JL:
			brmin = min_val;
			brmax = min(val->ival - 1, max_val);
			ftmin = max(val->ival, min_val);
			ftmax = max_val;
			break;
		case IT_JG:
			ftmin = min_val;
			ftmax = min(val->ival, max_val);
			brmin = max(val->ival + 1, min_val);
			brmax = max_val;
			break;
		case IT_JLE:
			brmin = min_val;
			brmax = min(val->ival, max_val);
			ftmin = max(val->ival + 1, min_val);
			ftmax = max_val;
			break;
		case IT_JZ:
			if ((val->ival < min_val) || (val->ival > max_val)) {
				// this is actually an unconditional jump. it will never be executed.
				brmin = 1;
				brmax = 0;
				ftmin = min_val;
				ftmax = max_val;
			} else if (val->ival == min_val) {
				brmin = min_val;
				brmax = min_val;
				ftmin = min_val + 1;
				ftmax = max_val;
			} else if (val->ival == max_val) {
				brmin = max_val;
				brmax = max_val;
				ftmin = min_val;
				ftmax = max_val - 1;
			} else {
				//FIXME sometimes generates duplicates
				brmin = brmax = val->ival;
				ftmin = min_val;
				ftmax = val->ival - 1;
				ftmin2 = val->ival + 1;
				ftmax2 = max_val;
			}
			//assert(0);
			break;
		case IT_JNZ:
			if ((val->ival < min_val) || (val->ival > max_val)) {
				// this is actually an unconditional jump
				ftmin = 1;
				ftmax = 0;
				brmin = min_val;
				brmax = max_val;
			} else if (val->ival == min_val) {
				ftmin = min_val;
				ftmax = min_val;
				brmin = min_val + 1;
				brmax = max_val;
			} else if (val->ival == max_val) {
				ftmin = max_val;
				ftmax = max_val;
				brmin = min_val;
				brmax = max_val - 1;
			} else {
				//TODO
				ftmin = brmin = 1;
				ftmax = brmax = 0;
				//assert(0);
			}
			//assert(0);
			break;

		default:
			assert(0);
		}
		if (branch && brmin <= brmax) {
			for (int j = 0; j < 6; ++j) {
				if (j == par) {
					if (!(node->valRanges.valRange[j])) {
						node->valRanges.valRange[j] = valrange_new_init(vpool,
								brmin, brmax, vt);
					} else {
						valrange_newtuple(node->valRanges.valRange[j], brmin,
								brmax);
					}
				}
				else
					node->valRanges.valRange[j] = parent->valRanges.valRange[j];
			}
			node->hasVr = true;
		}
		else if (!branch && ftmin <= ftmax) {
			for (int j = 0; j < 6; ++j) {
				if (j == par) {
					if (!(node->valRanges.valRange[j])) {
						node->valRanges.valRange[j] = valrange_new_init(vpool,
								ftmin, ftmax, vt);
					} else {
						valrange_newtuple(node->valRanges.valRange[j], ftmin,
								ftmax);
					}
					if (ftmin2 < ftmax2) {
						valrange_newtuple(node->valRanges.valRange[j], ftmin2,
								ftmax2);

					}
				}
				else
					node->valRanges.valRange[j] = parent->valRanges.valRange[j];
			}
			node->hasVr = true;

		}
	}
}
void new_jcc_unknown(VariantsNode* node) {
	node->fallthrough = node_new();
	node->fallthrough->parent = node;
	node->branch = node_new();
	node->branch->parent = node;
	node->expr = 0;
	node->instr = 0;
	copyValRanges(node->branch);
	copyValRanges(node->fallthrough);

}
void new_jcc(VariantsNode* node, ExprNode* expr, InstrType it, ValRangePool* vpool)
{
	if (!expr)
		fprintf(stderr, "Debug\n");
	if (!instrIsJcc(it)) assert(0);
	if ((node->fallthrough) || (node->branch)) assert (0);
	node->fallthrough = node_new();
	node->fallthrough->parent = node;
	node->branch = node_new();
	node->branch->parent = node;

	node->expr=expr;
	node->instr = it;


	// update valranges of children
	if (node->hasVr && node->expr) {
		calcValRanges(node->branch, vpool);
		calcValRanges(node->fallthrough, vpool);

	}
}






static
int tree_toStringLevel(VariantsNode* node, char* buf, int level, int off)
{

	if (level == -1 || node == 0)
		return off;
	else if (level==0)
	{
		if (instrIsJcc(node->instr)) {
			ExprNode* par = 0;
			ExprNode* value = 0;
			if (node->expr) {
				par = &(node->expr->p->n[node->expr->left]);
				value = &(node->expr->p->n[node->expr->right]);
			assert (par->type==NT_Par);
			assert (value->type==NT_Const);
			}
			off += sprintf(buf + off, "(%s %s %d)", instrName(node->instr, 0),
					par ? expr_toString(par) : 0, value ? value->ival : 0);

		}
		else if (node->instr == IT_RET)
		{
			off += sprintf(buf+off, "(%s)", instrName(node->instr, 0));

		}
		if (node->valRanges.valRange[0])
			off += sprintf(buf + off, " [par0 %s] ",
					valrange_toString(node->valRanges.valRange[0]));
		if (node->valRanges.valRange[1])
			off += sprintf(buf + off, " [par1 %s] ",
					valrange_toString(node->valRanges.valRange[1]));

		/*
		if (node->valRanges.valRange[1])
			off += sprintf(buf + off, " [par1 %s] ",
					valrange_toString(node->valRanges.valRange[1]));
		 */
		else return off;
	}
	else
	{
		off= tree_toStringLevel(node->branch, buf, level-1, off);
		off= tree_toStringLevel(node->fallthrough, buf, level-1, off);

	}

	return off;
}

char *tree_toString(VariantsNode* node)
{
#define bufsize (2000)
	static char buf [bufsize];
	int depth = tree_depth(node);
	int off = 0;
	for (int i = 0; i < depth; ++i)
	{
		off = tree_toStringLevel(node, buf, i, off);

		off += sprintf(buf + off, "\n\n");


	}
	assert (off<bufsize);
	return buf;
}

VariantsNode *nextNode(VariantsNode* node)
{
	VariantsNode* retNode = 0;
	if (node==0) return 0;
	switch (node->instr)
	{
	case IT_JZ:
	case IT_JNZ:
	case IT_JGE:
	case IT_JG:
	case IT_JLE:
	case IT_JL:
		if ((node->branch->instr == 0) && (node->fallthrough->instr == 0))
		{
			assert(0);
		}
		if (node->branch->instr == 0)
			retNode = node->branch;
		else if (node->fallthrough->instr == 0)
			retNode = node->fallthrough;
		else
			retNode=nextNode(node->parent);
		break;

	case IT_RET:
		retNode = nextNode(node->parent);
		break;
	default: assert(0);
	}

	return retNode;
}

VariantsNode *nextNodeBranch(VariantsNode* node, int branch)
{
	if (instrIsJcc(node->instr))
	{
		if (branch)
			return node->branch;
		return node->fallthrough;

	}
	assert (0);
	return 0;
}

void copyNode (VariantsNode* dst, VariantsNode* src, ValRangePool* vpool)
{
	assert(dst);
	dst->instr = src->instr;
	dst->expr = src->expr;
	dst->copied=true;
	for (int i = 0; i <= 6; ++i)
		dst->valRanges.valRange[i] = 0;
	dst->fallthrough = 0;
	dst->branch = 0;

	if (dst->parent&&dst->parent->valRanges.valRange[0])
	{
		calcValRanges(dst, vpool);
	}
	if (src->branch){
		dst->branch = node_new();
		dst->branch->parent = dst;

		copyNode(dst->branch, src->branch, vpool);
	}
	if (src->fallthrough){
		dst->fallthrough = node_new();
		dst->fallthrough->parent = dst;
		copyNode(dst->fallthrough, src->fallthrough, vpool);
	}
}

CBB2NodeList* new_cbb2nodelist(int size)
{
	CBB2NodeList* l = malloc(sizeof(CBB2NodeList) + size * sizeof(CBB2Node));
	l->size = size;
	l->used = 0;
	assert(l && (l->size > l->used));
	return l;
}
void freeCBB2NodeList(CBB2NodeList* list) {
	if (list)
		free(list);
}
void addCBB2Node(CBB2NodeList* list, CBB* cbb, VariantsNode* node) {
	assert(list && (list->used < list->size));

	CBB2Node* ret = list->cbb2Node + list->used;
	list->used++;
	ret->cbb = cbb;
	ret->node = node;
}

VariantsNode* getNode(CBB2NodeList* list, CBB* cbb) {
	for (int i = 0; i < list->used; ++i) {
		CBB2Node* cbb2node = &(list->cbb2Node[i]);
		if (cbb2node->cbb == cbb)
			return cbb2node->node;
	}
	return 0;
}
CBB* getCBB(CBB2NodeList* list, VariantsNode* node) {
	for (int i = 0; i < list->used; ++i) {
		CBB2Node* cbb2node = &(list->cbb2Node[i]);
		if (cbb2node->node == node)
			return cbb2node->cbb;
	}
	return 0;
}

static VariantsNode* getNextVariant(VariantsNode* node) {
	VariantsNode* ret = 0;

	// are we in a leaf?
	if (node->branch == 0 && node->fallthrough == 0) {
		if (node->hasVr)
			return node;
	}
	else {
		// search left subtree
		ret = getNextVariant(node->branch);
		if (ret)
			return ret;
		// search right subtree
		ret = getNextVariant(node->fallthrough);
		if (ret)
			return ret;
	}
	// if there is no leave with data in subtree, do backtracking
	VariantsNode* tmp = 0;
	while (!ret) {
		do {
			if (!node->parent)
				return 0;
			tmp = node;
			node = node->parent;

		} while (node->fallthrough == tmp);
		ret = getNextVariant(node->fallthrough);
	}
	return ret;


	assert(0);
	return 0;
}

VariantsNode* nextVariant(VariantsNode* node) {
	if (!tree_hasVariants(getRoot(node)))
		return 0;
	VariantsNode* n1 = 0;
	if (!(node->branch && node->fallthrough))
	{
		if (!node->parent)
			return 0;
		//n1 = node;
		do {
			if (!node->parent)
				return 0;
			n1 = node;
			node = node->parent;

		} while (node->fallthrough == n1);
		node = node->fallthrough;
	}

	return getNextVariant(node);
}

bool tree_hasVariants(VariantsNode* node) {
	if (node->branch && node->fallthrough)
		return (tree_hasVariants(node->branch)
				|| tree_hasVariants(node->fallthrough));
	else
		return node->hasVr;
}

