#ifndef _MANAGER_H_INCLUDED
#	define _MANAGER_H_INCLUDED

#	include "database.h"
#	include "error.h"

typedef struct {
	database_t database;
} manager_t;

result_t manager_open(manager_t *manager);
result_t manager_install_by_name(manager_t *manager, const char *name, const char *destination);
result_t manager_install_by_path(manager_t *manager, const char *path, const char *destination);
void manager_close(manager_t *manager);

#endif
