#ifndef _FLIST_H_INCLUDED
#	define _FLIST_H_INCLUDED

#	include "error.h"

typedef struct {
	char **paths;
	unsigned int count;
} file_list_t;

void file_list_new(file_list_t *list);
result_t file_list_add(file_list_t *list, const char *path);
void file_list_free(file_list_t *list);

#endif
