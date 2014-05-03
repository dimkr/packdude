#ifndef _DATABASE_H_INCLUDED
#	define _DATABASE_H_INCLUDED

#	include <sqlite3.h>
#	include "result.h"

#	define DATABASE_CREATION_QUERY \
	"BEGIN TRANSACTION;\n" \
	"CREATE TABLE packages (name TEXT,\n" \
	"                       version TEXT,\n" \
	"                       file_name TEXT,\n" \
	"                       arch TEXT,\n" \
	"                       deps TEXT,\n" \
	"                       reason TEXT,\n" \
	"                       id INTEGER PRIMARY KEY);\n" \
	"CREATE TABLE files (package TEXT,\n" \
	"                    path TEXT,\n" \
	"                    id INTEGER PRIMARY KEY);\n" \
	"COMMIT;"

#	define MAX_SQL_QUERY_SIZE (2048)

typedef int package_field_t;

enum package_fields {
	PACKAGE_FIELD_NAME      = 0,
	PACKAGE_FIELD_VERSION   = 1,
	PACKAGE_FIELD_FILE_NAME = 2,
	PACKAGE_FIELD_ARCH      = 3,
	PACKAGE_FIELD_DEPS      = 4,
	PACKAGE_FIELD_REASON    = 5
};

#	define METADATA_FIELDS_COUNT (6)
#	define INSTALLATION_DATA_FIELDS_COUNT (7)


typedef struct {
	char *_fields[INSTALLATION_DATA_FIELDS_COUNT];
} package_info_t;

#	define p_name _fields[PACKAGE_FIELD_NAME]
#	define p_version _fields[PACKAGE_FIELD_VERSION]
#	define p_file_name _fields[PACKAGE_FIELD_FILE_NAME]
#	define p_arch _fields[PACKAGE_FIELD_ARCH]
#	define p_deps _fields[PACKAGE_FIELD_DEPS]
#	define p_reason _fields[PACKAGE_FIELD_REASON]

typedef struct {
	sqlite3 *handle;
	const char *path;
} database_t;

typedef int (*query_callback_t) (void *arg,
                                 int count,
                                 char **values,
                                 char **names);

result_t database_open_read(database_t *database, const char *path);
result_t database_open_write(database_t *database, const char *path);
void database_close(database_t *database);

result_t database_get(database_t *database,
                      const char *name,
                      package_info_t *info);

#	define database_get_metadata database_get
#	define database_get_installation_data database_get

result_t database_set_installation_data(database_t *database,
                                        const package_info_t *info);
result_t database_remove_installation_data(database_t *database,
                                           const char *package);

void package_info_free(package_info_t *info);

result_t database_register_path(database_t *database,
                                const char *path,
                                const char *package);

result_t database_unregister_path(database_t *database, const char *path);

result_t database_for_each_inst_package(database_t *database,
                                        const query_callback_t callback,
                                        void *arg);

result_t database_for_each_file(database_t *database,
                                const char *name,
                                const query_callback_t callback,
                                void *arg);

#endif