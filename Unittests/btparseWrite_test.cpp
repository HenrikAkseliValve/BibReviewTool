// Test the btparseWrite module.
// 
// Important since extending btparse is not supported (in fact it is warmed against)!

#include <CppUTest/TestHarness.h>
#include <string.h>
#include "../Source/btparseWrite.h"
#include "../Source/btparseStaticAlloc.h"

// File name for the test file written.
//TODO: this assumes that /tmp folder exists in NON-unix systems.
const char *TestFileName = "/tmp/test.bib";

TEST_GROUP(btparseWrite_group1)
{
	AST* manual_root;
	AST* manual_field;

	// This is initialization stuff for every test.
	// REPEAT: This will be run before every TEST!
	void setup()
	{

		// Create the AST manually.
		// NOTE: Library btparse assumes text value is malloced.
		char *titlename = strdup("Book of Cow");
		AST *temp = bt_alloc_ast(NULL
			                      ,NULL
			                      ,(char*)TestFileName
			                      ,titlename
			                      ,BTE_UNKNOWN
			                      ,BTAST_STRING
		);

		this->manual_field = bt_alloc_ast(NULL
                       ,temp
		                   ,(char*)TestFileName
		                   ,(char*)"title"
		                   ,BTE_UNKNOWN
		                   ,BTAST_FIELD
		);
		temp = bt_alloc_ast(this->manual_field
                       ,NULL
			                 ,(char*)TestFileName
			                 ,(char*)"bar"
			                 ,BTE_UNKNOWN
			                 ,BTAST_KEY
		);
		this->manual_root = bt_alloc_ast(NULL
		                                ,temp
		                                ,(char*)TestFileName
		                                ,(char*)"FOO"
		                                ,BTE_REGULAR
		                                ,BTAST_ENTRY
		);


	}
	// This is opposite for initialization IF needed.
	void teardown()
	{
		free(this->manual_root->down->right->down->text);
		free(this->manual_root->down->right->down);
		free(this->manual_root->down->right);
		free(this->manual_root->down);
		free(this->manual_root);
	}
};

IGNORE_TEST(btparseWrite_group1, test_bt_save_file)
{

	// Test bt_save_file function here.
	bt_save_file(this->manual_root);

	// Verify the file was saved successfully
	FILE* savedFile = fopen(TestFileName, "r");
	CHECK_TRUE(savedFile != NULL);

	char buffer[1024];
	memset(buffer, 0, sizeof(buffer));
	size_t bytesRead = fread(buffer, 1, sizeof(buffer), savedFile);

	STRCMP_EQUAL("@FOO{bar,\n"
	             "title = {Book of Cow},\n"
	             "}\n"
              ,buffer);

	fclose(savedFile);
}

TEST(btparseWrite_group1, test_bt_set_field_value)
{
	// Reset field value of title to something else.
	char catmad[]="Cat mad.";
	bt_set_field_value(this->manual_field,catmad);
	STRCMP_EQUAL(catmad,this->manual_root->down->right->down->text);
}

TEST(btparseWrite_group1, test_bt_add_field)
{
	char author[] = "Author";
	char name[] = "Cow Mack Cow";
	AST *newfield = bt_add_field(this->manual_root,author,name);

	CHECK_TEXT(this->manual_root->down->right->right,"Field is not accessible?");
	STRCMP_EQUAL(author,this->manual_root->down->right->right->text);
	CHECK_TEXT(this->manual_root->down->right->right->down,"Field value is not set?");
	STRCMP_EQUAL(name,this->manual_root->down->right->right->down->text);

	// Free allocations done by bt_add_field manually.
	free(newfield->down);
	free(newfield);
}

TEST(btparseWrite_group1, test_bt_add_entry)
{
	char entrytype[]="manual";
	char entryname[]="TC_MANUAL";

	AST *newentry = bt_add_entry(this->manual_root,entrytype,entryname);

	CHECK_TEXT(this->manual_root->right,"Entry is not accessible?");
	STRCMP_EQUAL(this->manual_root->right->text,entrytype);
	CHECK_TEXT(this->manual_root->right->down,"Entry key is not accessible?");
	STRCMP_EQUAL(this->manual_root->right->down->text,entryname);

	// Free alocations done by bt_add_entry manually.
	free(newentry->down);
	free(newentry);

}
