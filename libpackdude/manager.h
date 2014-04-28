#ifndef _MANAGER_H_INCLUDED
#	define _MANAGER_H_INCLUDED

#	include <stdbool.h>
#	include "database.h"

typedef struct {
	database_t database;
} manager_t;

bool manager_open(manager_t *manager);
bool manager_install_by_name(manager_t *manager, const char *name, const char *destination);
bool manager_install_by_path(manager_t *manager, const char *path, const char *destination);
void manager_close(manager_t *manager);

#endif
