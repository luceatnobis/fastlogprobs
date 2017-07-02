#include "jansson.h"

int _glyph_lookup(wchar_t);

wchar_t *glyphs = L"ᚠᚢᚦᚩᚱᚳᚷᚹᚻᚾᛁᛄᛇᛈᛉᛋᛏᛒᛖᛗᛚᛝᛟᛞᚪᚫᚣᛡᛠ";
int codepoints[] = {
    5792, 5794, 5798, 5801, 5809, 5811, 5815, 5817, 5819, 5822, 5825, 5828,
    5831, 5832, 5833, 5835, 5839, 5842, 5846, 5847, 5850, 5853, 5855, 5854,
    5802, 5803, 5795, 5857, 5856
};

typedef struct NGRAM_TREE {
    uint64_t *counts;
	struct NGRAM_TREE *node[29];
} NGRAM_TREE;

NGRAM_TREE* ngram_tree_init(){
	NGRAM_TREE *tree = calloc(1, sizeof(NGRAM_TREE));
	return tree;
}

void ngram_tree_free(NGRAM_TREE *tree){
    int i;
    if(tree->counts){
        free(tree->counts);
    }
    for(i = 0; i < 29; i++){
        if(tree->node[i]){
            ngram_tree_free(tree->node[i]);
        }
    }
    free(tree);
}

// int64_t _ngram_lookup(const NGRAM_TREE *tree, const wchar_t *ngram, int n, int len){
int64_t _ngram_lookup(const NGRAM_TREE *tree, const int8_t *ngram, int n, int len){
    int node_index = _glyph_lookup(ngram[len - n]);
    if(n == 0){
        return tree->counts[node_index];
    } else {
        if(!tree->node[node_index]){
            return -1;
        }
        return _ngram_lookup(tree->node[node_index], ngram, n-1, len);
    }
}

uint64_t ngram_lookup(const NGRAM_TREE *tree, const int8_t *ngram, int n){
	return _ngram_lookup(tree, ngram, n, n);
}

void _update_lookup(NGRAM_TREE *tree, int *ngram, int n, int len){
    unsigned char rune_index = ngram[len - n];
    if(n == 0){
        if(!tree->counts){
            tree->counts = calloc(29, sizeof(uint64_t));
        }
        tree->counts[rune_index]++;
    } else {
        if(!tree->node[rune_index]){
            tree->node[rune_index] = calloc(1, sizeof(NGRAM_TREE));
        }
        return _update_lookup(tree->node[rune_index], ngram, n-1, len);
    }
}

void ngram_tree_update(NGRAM_TREE *tree, int *ngram, int n){
    _update_lookup(tree, ngram, n-1, n);
}

void _ngram_merge_trees(NGRAM_TREE *t1, const NGRAM_TREE* t2){
    int i;
    if(t2->counts){
        if(!t1->counts){  // t2 has counts, but t1 has no mem allocated
            t1->counts = calloc(29, sizeof(uint64_t));
        }
        for(i = 0; i < 29; i++){
            t1->counts[i] += t2->counts[i];
        }
    }
    for(i = 0; i < 29; i++){
        if(!t2->node[i]){
            continue;
        }
        if(!t1->node[i]){
            t1->node[i] = calloc(1, sizeof(NGRAM_TREE));
        }
        _ngram_merge_trees(t1->node[i], t2->node[i]);
    }
}

/*
void dump_tree_to_json(NGRAM_TREE* tree){
    json_t *json = json_object();
    free(json);
}
*/

inline int _glyph_lookup(int codepoint){
    for(int i = 0; i < 29; i++){
        if(codepoints[i] == codepoint){
            return i;
        }
    }
    return -1;
}
