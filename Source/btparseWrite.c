#include <stdlib.h>
#include <btparse.h>
#define __STDC_ALLOC_LIB__
#define __STDC_WANT_LIB_EXT2__ 1
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <limits.h>
#include <errno.h>
#include "stdlibext.h"
#include "btparseStaticAlloc.h"
#include "btparseWrite.h"

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
	char *fieldnamecopy = strdup(fieldname);
	if(!fieldnamecopy) goto FREE_ERROR2;
	lastfield->right = bt_alloc_ast(NULL
	                               ,newvalue
	                               ,entry->filename
	                               ,fieldnamecopy
	                               ,BTE_UNKNOWN
	                               ,BTAST_FIELD
	                               );
	if(!lastfield->right) goto FREE_ERROR3;

	return lastfield->right;

	// If one malloc fails after first malloc jump here
	// to free resources in right order.
	FREE_ERROR3:
	free(fieldnamecopy);
	FREE_ERROR2:
	free(newvalue);
	FREE_ERROR1:
	free(copyvalue);
	return NULL;

}

void bt_set_field_value(AST *field,char *fieldvalue){
	// Value of the field is down from the field node.
	bt_set_text(field->down,fieldvalue);
}

/*
Check is iovec too big and needs to be flushed. Updates index (vecindex)
indexing iovec array (vecbuff) if flush happens. Parameter upcoming is
used tell how many vecs are upcoming after this flush. This is used in
flush decision making. Also if number of characters to write is over
recommended buffer size then flush.
*/
static void writevFlushCheck(int fd,struct iovec *vecbuff,int *vecindex,int upcoming,int maxvec,size_t *buffersize,size_t bufferrecomsize){
	if((*vecindex)+upcoming>maxvec || (*buffersize)>=bufferrecomsize){
		if(writev(fd,vecbuff,*vecindex)<0){
			//TODO: Error management.
		}

		// Reset the IO vec index and buffer size counter.
		*vecindex = 0;
		*buffersize = 0;
	}
}

char *bt_write_file(int fd,AST *root){

	// Root node/lowest level is ether:
	//   * entry,
	//   * macro,
	//   * comment,
	//   * preamble
	//
	// Entry in the bib file should be constructed by:
	//    @<entry type>{<entry key>
	//                 ,<field name>=<field value>
	//                 ┆
	//                 ,<field name>=<field value>
	//                 }
	// or same but in one line.
	//
	// Use writev to write. Send iovec buffer out if 4096 is send.
	// Number of iovecs is the minimum of quarter of recommended
	// and system limit. It is pretty much guaranteed that not all
	// strings to be one byte. If such case would happen then it
	// is weird edge case. Hence, quarter should be safe.
	// TODO: Check buffer size by fstat.
	const size_t bufferrecomsize = 4096;
	#define MIN(A,B) A<=B?A:B
	struct iovec vecbuff[MIN(bufferrecomsize/4,IOV_MAX)];
	int vecindex = 0;
	size_t buffersize = 0;

	// Jump to next entry without handling macro, comment, or preamble.
	// TODO: Handle macro, comment, or preamble.
	AST *iteentry = NULL;
	while((iteentry = bt_next_entry(root,iteentry))!=NULL){

		// Technically bt_entry_* do not give const but not sure
		// if something would broke if null byte would be used
		// to optimize out '{' and '\n' would be just added to
		// null position.
		const char *entrytype = bt_entry_type(iteentry);
		const char *entrykey = bt_entry_key(iteentry);

		// Check for flush.
		writevFlushCheck(fd,vecbuff,&vecindex,5,sizeof(vecbuff)/sizeof(struct iovec),&buffersize,bufferrecomsize);

		// Write "@<entry type>{<entry key>\n"
		vecbuff[vecindex].iov_base = "@";
		vecbuff[vecindex].iov_len = 1;
		buffersize += 1;
		vecindex++;

		vecbuff[vecindex].iov_base = (void*)entrytype;
		vecbuff[vecindex].iov_len = strlen(entrytype);
		buffersize += vecbuff[vecindex].iov_len;
		vecindex++;

		vecbuff[vecindex].iov_base = "{";
		vecbuff[vecindex].iov_len = 1;
		buffersize += 1;
		vecindex++;

		vecbuff[vecindex].iov_base = (void*)entrykey;
		vecbuff[vecindex].iov_len = strlen(entrykey);
		buffersize += vecbuff[vecindex].iov_len;
		vecindex++;

		vecbuff[vecindex].iov_base = "\n";
		vecbuff[vecindex].iov_len = 1;
		buffersize += 1;
		vecindex++;

		AST *field = NULL;
		char *fieldname;
		while((field = bt_next_field(iteentry,field,&fieldname))){

			char *value;
			bt_nodetype valuetype;
			AST *valuenode = NULL;

			//TODO: Handle multi node value.
			valuenode = bt_next_value(field,valuenode,&valuetype,&value);

			// Check for flush.
			writevFlushCheck(fd,vecbuff,&vecindex,5,sizeof(vecbuff)/sizeof(struct iovec),&buffersize,bufferrecomsize);


			// Write "\t<field name>=<field value>\n"
			vecbuff[vecindex].iov_base = "\t";
			vecbuff[vecindex].iov_len = 1;
			buffersize += 1;
			vecindex++;

			vecbuff[vecindex].iov_base = fieldname;
			vecbuff[vecindex].iov_len = strlen(fieldname);
			buffersize += vecbuff[vecindex].iov_len;
			vecindex++;

			vecbuff[vecindex].iov_base = "=";
			vecbuff[vecindex].iov_len = 1;
			buffersize += 1;
			vecindex++;

			vecbuff[vecindex].iov_base = value;
			vecbuff[vecindex].iov_len = strlen(value);
			buffersize += vecbuff[vecindex].iov_len;
			vecindex++;

			vecbuff[vecindex].iov_base = "\n";
			vecbuff[vecindex].iov_len = 1;
			buffersize += 1;
			vecindex++;
		}

		// Check for flush.
		writevFlushCheck(fd,vecbuff,&vecindex,1,sizeof(vecbuff)/sizeof(struct iovec),&buffersize,bufferrecomsize);

		// Write ending bracket for entry "}\n".
		vecbuff[vecindex].iov_base = "}\n";
		vecbuff[vecindex].iov_len = 2;
		buffersize += 1;
		vecindex++;
	}

	// Force flush since there is noting more to write.
	if(writev(fd,vecbuff,vecindex)<0){
		//TODO: Error management.
	}

	return NULL;
}

char *bt_save_file(const char* filepath, AST *root){
	// Since this is a C file use POSIX standard.
	// Systems which do not support POSIX use
	// porting library or porting module.

	//TODO: Check that filepath exists? If not just call write?

	// Linux has O_TMPFILE to create temporary file more securely.
	// Has to handle error to make back up support.

	// Filename given can be relative to local folder hence
	// check is folder temporary file created just for dot.
	const char *lastslashloc = strchr(filepath,'/');
	const char *folderpath;
	// If lastslashloc is null then given filename was in current
	// working directory hence folder path is '.'.
	if(!lastslashloc) folderpath = ".";
	else{
		folderpath = filepath;
		// Temporary force out of constant.
		*(char*)lastslashloc = '\0';
	}

	int fd = open(folderpath,O_TMPFILE | O_WRONLY,S_IRUSR | S_IWUSR);

	// Return '/' to nulled location to keep constant.
	if(lastslashloc) *(char*)lastslashloc = '/';

	if(fd!=-1){
		bt_write_file(fd,root);
		// Rename old version to have ".backup" ending.
		// NOTE: Constant dotbackup is initialized as string so
		//       sizeof(dotbackup) includes the NULL byte ending!
		{
			const char dotbackup[] = ".backup";
			const size_t filepathlen = strlen(filepath);
			char filepathback[filepathlen+sizeof(dotbackup)];
			memcpy(filepathback,filepath,filepathlen);
			memcpy(filepathback+filepathlen,dotbackup,sizeof(dotbackup));
			rename(filepath,filepathback);
		}
		// Link the temporary file to save file.
		// Do not use AT_EMPTY_PATH as it is too much of a
		// pain to check to be useful. (Call to capabilities and checking
		// for failure is not worth it.)
		//
		// As previously sizeof(symfdfolder) includes the null ending but
		// output of i32toa does not set null ending so it is manually
		// done.
		{
			const char symfdfolder[] = "/proc/self/fd/";
			size_t intlen = i32toalen(fd);
			uint8_t symfdpath[sizeof(symfdfolder)+intlen];
			memcpy(symfdpath,symfdfolder,sizeof(symfdfolder)-1);
			i32toa(fd,symfdpath+sizeof(symfdfolder)-1,intlen);
			symfdpath[sizeof(symfdfolder)+intlen-1] = '\0';
			linkat(AT_FDCWD,(char*)symfdpath,AT_FDCWD,filepath,AT_SYMLINK_FOLLOW);
		}
		// Remove the old bibfile.
		{
			;
		}
		#ifndef CAP_DAC_READ_SEARCH
			//#error AT_EMPTY_PATH is defined in Linux 2.6.39. Some reason it is not defined. Different OS then Linux?
		#endif
		//linkat(fd,"",AT_FDCWD,filepath,AT_EMPTY_PATH); // Should work but should be tested without overwrite.
	}
	else{
		// Error handling as mention. If O_TMPFILE related errno are:
		//   - EINVAL O_WRONLY or O_RDWR was not given
		//     ... do not check since we do give the O_WRONLY flag... ,
		//   - EISDIR then kernel does not support O_TMPFILE but pathname was correct,
		//   - ENOENT then kernel does not support O_TMPFILE but pathname was not correct
		//     ... do not check since path is wrong... ,
		//   - EOPNOTSUPP filesystem does not support O_TMPFILE
		//
		// Handle so that if errno is not certain type then return an error otherwise let through
		// for backup.
		if(errno!=EISDIR  && errno!=EOPNOTSUPP){
			return "Couldn't create temporary file !\n";
		}
	}


	close(fd);
#if 0
	// Possible back up support using mkstemps?
	//TODO:
	char tempfile[] = "/tmp/bibtex.XXXXXX.bib";
	int fd = mkstemps(tempfile,4);
#endif

	// There was no error so return NULL.
	return NULL;
}
