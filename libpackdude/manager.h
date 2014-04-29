#ifndef _MANAGER_H_INCLUDED
#	define _MANAGER_H_INCLUDED

#	include <stdbool.h>
#	include "database.h"
#	include "error.h"

typedef struct {
	const char *prefix;
	database_t database;
} manager_t;

result_t manager_open(manager_t *manager, const char *prefix);
result_t manager_install_by_name(manager_t *manager, const char *name);
result_t manager_install_by_path(manager_t *manager, const char *path);
result_t manager_remove(manager_t *manager, const char *name, const bool force);
result_t manager_cleanup(manager_t *manager);
void manager_close(manager_t *manager);

#endif
