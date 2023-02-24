/*
< This header has static function for allocating btparse AST nodes.
< This header was created so that btparseWrite test would have access
< to it without compromising static.
*/
#ifndef BT_PARSE_STATIC_ALLOC_H
#define BT_PARSE_STATIC_ALLOC_H

#include <stdlib.h>
#include <btparse.h>
#include <stddef.h>
#include <assert.h>


/*
Allocate a new AST node.
*/
static AST *bt_alloc_ast(AST *right,AST *down,char *filename,char *text,bt_metatype metatype,bt_nodetype nodetype){

	// Sanity checks that header has not changed.
	// Since, what btparseWrite is doing is unsupported usage
	// this makes sure bugs caused by structure changes are
	// known immediately.
	//
	// This manually check that padding + offsetting rules are followed as expected.
	static_assert(offsetof(AST,right)==0,"AST member \"right\" not what expected!");
	static_assert(offsetof(AST,down)==sizeof(struct _ast*),"AST member \"down\" not what expected!");
	static_assert(offsetof(AST,filename)==2*sizeof(struct _ast*),"AST member \"filename\" not what expected!");
	static_assert(offsetof(AST,line)==2*sizeof(struct _ast*)+sizeof(char*),"AST member \"line\" not what expected!");
	static_assert(offsetof(AST,offset)==2*sizeof(struct _ast*)+sizeof(char*)+sizeof(int),"AST member \"offset\" not what expected!");
	static_assert(offsetof(AST,nodetype)==2*sizeof(struct _ast*)+sizeof(char*)+2*sizeof(int),"AST member \"nodetype\" not what expected!");
	static_assert(offsetof(AST,metatype)==2*sizeof(struct _ast*)+sizeof(char*)+2*sizeof(int)+sizeof(bt_nodetype),"AST member \"metatype\" not what expected!");
	static_assert(offsetof(AST,text)==2*sizeof(struct _ast*)+sizeof(char*)+2*sizeof(int)+sizeof(bt_nodetype)+sizeof(bt_metatype),"AST member \"text\" not what expected!");

	// Allocation
	AST *newnode=(AST*)malloc(sizeof(AST));
	if(newnode) return NULL;

	// Initialization
	// Member line is Initialize to -1 as we do
	// not yet know where it ends up in the file.
	//
	// NOTE: Function bt_set_text calls free on a old value.
	//       Hence, since we do not have old value just
	//       manually add it.
	//       It also since free is used then uses strdup to
	//       make memory allocation. Better to manually.
	newnode->right=right;
	newnode->down=down;
	newnode->filename=filename;
	newnode->line=-1;
	newnode->metatype=metatype;
	newnode->nodetype=nodetype;
	newnode->text=text;
	return newnode;
}

#endif /* BT_PARSE_STATIC_ALLOC_H */
