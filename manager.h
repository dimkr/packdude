#ifndef _MANAGER_H_INCLUDED
#	define _MANAGER_H_INCLUDED

#	include "repo.h"
#	include "database.h"
#	include "stack.h"
#	include "result.h"

#	define DEFAULT_PREFIX "/"

#	define REPOSITORY_URL "ftp://dimakrasner.com/packdude"

#	define PACKAGE_ARCHIVE_DIR "."VAR_DIR"/packdude/archive"

#	define INSTALLATION_DATA_DATABASE_PATH "."VAR_DIR"/packdude/install.sqlite3"

#	define INSTALLATION_REASON_USER "user"
#	define INSTALLATION_REASON_DEPENDENCY "dependency"

#	define ARCHITECTURE_INDEPENDENT "all"

typedef struct {
	repo_t repo;
	database_t avail_packages;
	database_t inst_packages;
	node_t *inst_stack;
} manager_t;

typedef result_t (*dependency_callback_t)(const char *name, void *arg);

result_t manager_new(manager_t *manager, const char *prefix);
void manager_free(manager_t *manager);

result_t manager_for_each_dependency(manager_t *manager,
                                     const char *name,
                                     const dependency_callback_t callback,
                                     void *arg);

result_t manager_fetch(manager_t *manager,
                       const char *name,
                       const char *reason);

result_t manager_remove(manager_t *manager, const char *name);

result_t manager_is_installed(manager_t *manager, const char *name);

result_t manager_cleanup(manager_t *manager);

#endif