#ifndef TREE_VARIANTS_H
#define TREE_VARIANTS_H

#include "common.h"


/*attributes:
 * type (constant/depends on param)
 * value (for constant imm, for dep expression)
*/


#define variantsParCount (6)

typedef struct _valRanges ValRanges;
typedef struct _variantsNode VariantsNode;
typedef struct _cbb2node CBB2Node;
typedef struct cbb2nodelist CBB2NodeList;

struct _valRanges {
	ValRange* valRange[variantsParCount];
};

struct _variantsNode {
	ExprNode* expr;
	InstrType instr;
	bool copied;
	bool hasVr;
	VariantsNode* parent;
	ValRanges valRanges;

	VariantsNode* branch;
	VariantsNode* fallthrough;

};

struct _cbb2node {
	struct _cbb2node* next;
	CBB* cbb;
	VariantsNode* node;
};

struct cbb2nodelist {
	int size;
	int used;
	CBB2Node cbb2Node[1];
};


// creates a new empty node
VariantsNode* tree_new(void);
void init_root(VariantsNode* root, ValRanges* vr);

void new_jcc_unknown(VariantsNode* node);

void new_jcc(VariantsNode*, ExprNode*, InstrType, ValRangePool*);

// frees entire tree recursively
void tree_free(VariantsNode*);

int tree_depth(VariantsNode*);

VariantsNode *nextNode(VariantsNode* node);

VariantsNode *nextNodeBranch(VariantsNode* node, int branch);

char *tree_toString(VariantsNode* node);

void copyNode (VariantsNode* dst, VariantsNode* src, ValRangePool*);

bool vrAreEqual(ValRanges* vr1, ValRanges* vr2);

CBB2NodeList* new_cbb2nodelist(int);
void freeCBB2NodeList(CBB2NodeList*);
void addCBB2Node(CBB2NodeList*, CBB*, VariantsNode*);
VariantsNode* getNode(CBB2NodeList*, CBB*);
CBB* getCBB(CBB2NodeList*, VariantsNode*);

VariantsNode* nextVariant(VariantsNode* node);
bool tree_hasVariants(VariantsNode* node);
VariantsNode* getRoot(VariantsNode* node);


#endif //TREE_VARIANTS_H
