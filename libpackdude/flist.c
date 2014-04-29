#include <string.h>
#include <stdlib.h>
#include "flist.h"

void file_list_new(file_list_t *list) {
	/* initialize the array */
	list->paths = NULL;
	list->count = 0;
}

result_t file_list_add(file_list_t *list, const char *path) {
	/* the return value */
	result_t result = RESULT_REALLOC_FAILED;

	/* the enlarged file list */
	file_list_t new_list;

	/* ignore the root directory */
	if (0 == strcmp("./", path))
		goto success;

	/* enlarge the file list */
	new_list.count = 1 + list->count;
	new_list.paths = realloc(list->paths, sizeof(char *) * new_list.count);
	if (NULL == new_list.paths)
		goto end;
	(void) memcpy(list, &new_list, sizeof(file_list_t));

	/* skip the leading "./" */
	path += 2;

	/* duplicate the path */
	--(new_list.count);
	list->paths[new_list.count] = strdup(path);
	if (NULL == list->paths[new_list.count]) {
		result = RESULT_STRDUP_FAILED;
		goto end;
	}

success:
	/* report success */
	result = RESULT_OK;

end:
	return result;
}

void file_list_free(file_list_t *list) {
	/* free the paths array */
	free(list->paths);
}
