/*
< This is a support module to write btparse entry.
< Library btparse has some functions to modify AST
< tree but it does not have way to write AST back
< to a file. Hence, this module.
*/
#ifndef BTPARSE_WRITE_H
#define BTPARSE_WRITE_H

#include<btparse.h>

#if defined(__cplusplus__) || defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/*
Adds an entry to given AST entry.
Gives back the new entry.
*/
AST *bt_add_entry(AST *last_entry,char *entry,char *key);
/*
Add field to the entry.
Gives back the field AST.
*/
AST *bt_add_field(AST *entry,const char *fieldname,char *fieldvalue);
/*
Edits given field value.
Gives back the field AST.
*/
void bt_set_field_value(AST *entry,char *fieldvalue);
/*
Save AST to the file. Save differs from writing by writing
temporary file which is swapped with the actual file target.
*/
void bt_save_file(AST *root);

#if defined(__cplusplus__) || defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif /* _BT_PARSE_WRITE_H_ */
