#include<stdlib.h>
#include<btparse.h>
#define __STDC_ALLOC_LIB__
#define __STDC_WANT_LIB_EXT2__ 1
#include<string.h>
#include"btparseStaticAlloc.h"
#include"btparseWrite.h"


AST *bt_add_entry(AST *last_entry,char *entry,char *key){

	// Allocate the entry key.
	//
	// Library btparse does actual give function for this
	// but it does not allocate new node it only changes
	// the value of already existing one. Also metatype and
	// nodetype do not have right value because of that.
	// Hence, manually allocate and set the values.
	AST* newkey = bt_alloc_ast(NULL
	                         ,NULL
	                         ,last_entry->filename
	                         ,key
	                         ,BTE_UNKNOWN
	                         ,BTAST_KEY
	                         );
	if(!newkey) return NULL;

	// Allocate new entry from heap.
	// Entries are order in the right and down is to inside the entry.
	last_entry->right = bt_alloc_ast(NULL
	                                ,newkey
	                                ,last_entry->filename
	                                ,entry
	                                ,BTE_UNKNOWN
	                                ,BTAST_ENTRY
	                                );

	// Allocation success check.
	if(!last_entry->right){
		free(newkey);
		return NULL;
	}

	return last_entry->right;
}
AST *bt_add_field(AST *entry,const char *fieldname,char *fieldvalue){

	// Get the first field and then find the last field.
	// Since we are just interested in last field ignore last parameter of
	// bt_next_field which gives the name of the found field.
	char *ignore;
	AST *lastfield = bt_next_field(entry,NULL,&ignore);
	while(lastfield->right != NULL) lastfield = lastfield->right;

	// Add value for the node which is down
	// from the field name.
	char *copyvalue = strdup(fieldvalue);
	if(!copyvalue) return NULL;

	AST *newvalue = bt_alloc_ast(NULL
	                            ,NULL
	                            ,entry->filename
	                            ,copyvalue
	                            ,BTE_UNKNOWN
	                            ,BTAST_STRING
	                            );
	if(!newvalue) goto FREE_ERROR1;


	// Adding the field.
	//
	// NOTE: Function bt_set_text calls free on a old value.
	//       Hence, since we do not have old value just
	//       manually add it.
	//       Also since free is used then use strdup to
	//       make memory allocation.
	lastfield->right = malloc(sizeof(AST));
	if(!lastfield) return NULL;
	lastfield->right->metatype = BTE_UNKNOWN;
	lastfield->right->nodetype = BTAST_FIELD;
	lastfield->right->filename = entry->filename;
	lastfield->right->right=NULL;
	lastfield->right->text = strdup(fieldname);

	char *fieldnamecopy = strdup(fieldname);
	lastfield->right = bt_alloc_ast(NULL
	                               ,newvalue
	                               ,entry->filename
	                               ,fieldnamecopy
	                               ,BTE_UNKNOWN
	                               ,BTAST_FIELD
	                               );

	return lastfield->right;

	FREE_ERROR1:
	free(copyvalue);
	return NULL;

}
void bt_set_field_value(AST *field,char *fieldvalue){
	// Value of the field is down from the field node.
	bt_set_text(field->down,fieldvalue);
}
void bt_save_file(AST *root){
}
