#include<stdlib.h>
#include<btparse.h>
#define __STDC_ALLOC_LIB__
#define __STDC_WANT_LIB_EXT2__ 1
#include<string.h>
#include"btparseWrite.h"

AST *bt_add_entry(AST *last_entry,char *entry,char *key){
	// Allocate new entry from heap.
	// Entries are order in the right and down is to inside the entry.
	last_entry->right = malloc(sizeof(AST));

	// Initialize the new entry.
	last_entry->right->metatype = BTE_REGULAR;
	last_entry->right->nodetype = BTAST_ENTRY;
	last_entry->right->filename = last_entry->filename;
	last_entry->right = NULL;
	last_entry->right->text = entry;

	// Library btparse does actual give function for this
	// but it does not allocate new node it only changes
	// the value of already existing one. Also metatype and
	// nodetype do not have right value because of that.
	// Hence, manually allocate and set the values.
	//
	// NOTE: Function bt_set_text calls free on a old value.
	//       Hence, since we do not have old value just
	//       manually add it.
	last_entry->right->down = malloc(sizeof(AST));
	last_entry->right->down->metatype = BTE_UNKNOWN;
	last_entry->right->down->nodetype = BTAST_KEY;
	last_entry->right->down->filename = last_entry->filename;
	last_entry->right->down->down = NULL;
	last_entry->right->down->right = NULL;
	last_entry->right->down->text = key;

	return last_entry->right;
}
AST *bt_add_field(AST *entry,const char *fieldname,char *fieldvalue){

	// Get the first field and then find the last field.
	// Since we are just interested in last field ignore last parameter of
	// bt_next_field which gives the name of the found field.
	char *ignore;
	AST *lastfield = bt_next_field(entry,NULL,&ignore);
	while(lastfield->right != NULL) lastfield = lastfield->right;

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

	// Add value for the node which is down
	// from the field name.
	lastfield->right->down = malloc(sizeof(AST));
	if(!lastfield) return NULL;
	lastfield->right->down->metatype = BTE_UNKNOWN;
	lastfield->right->down->nodetype = BTAST_STRING;
	lastfield->right->down->filename = entry->filename;
	lastfield->right->down->right = NULL;
	lastfield->right->down->down = NULL;
	lastfield->right->down->text = strdup(fieldvalue);

	return lastfield->right;
}
void bt_set_field_value(AST *field,char *fieldvalue){
	// Value of the field is down from the field node.
	bt_set_text(field->down,fieldvalue);
}
void bt_save_file(AST *root){
}
