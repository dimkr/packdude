#ifndef _DATABASE_H_INCLUDED
#	define _DATABASE_H_INCLUDED

#	include <sqlite3.h>

#	include "result.h"

/*!
 * @defgroup database Database
 * @brief Database access
 * @{ */

/*!
 * @def METADATA_DATABASE_CREATION_QUERY
 * @brief The SQL query used to initialize a metadata database */
#	define METADATA_DATABASE_CREATION_QUERY \
	"BEGIN TRANSACTION;\n" \
	"CREATE TABLE packages (name TEXT UNIQUE NOT NULL,\n" \
	"                       version TEXT NOT NULL,\n" \
	"                       desc TEXT NOT NULL,\n" \
	"                       file_name TEXT UNIQUE NOT NULL,\n" \
	"                       arch TEXT NOT NULL,\n" \
	"                       deps TEXT NOT NULL,\n" \
	"                       id INTEGER PRIMARY KEY);\n" \
	"COMMIT;"

/*!
 * @def INSTALLATION_DATA_DATABASE_CREATION_QUERY
 * @brief The SQL query used to initialize a installation data database */
#	define INSTALLATION_DATA_DATABASE_CREATION_QUERY \
	"BEGIN TRANSACTION;\n" \
	"CREATE TABLE packages (name TEXT UNIQUE NOT NULL,\n" \
	"                       version TEXT NOT NULL,\n" \
	"                       desc TEXT NOT NULL,\n" \
	"                       file_name TEXT UNIQUE NOT NULL,\n" \
	"                       arch TEXT NOT NULL,\n" \
	"                       deps TEXT NOT NULL,\n" \
	"                       reason TEXT NOT NULL,\n" \
	"                       id INTEGER PRIMARY KEY);\n" \
	"CREATE TABLE files (package TEXT,\n" \
	"                    path TEXT,\n" \
	"                    id INTEGER PRIMARY KEY);\n" \
	"COMMIT;"

/*!
 * @def MAX_SQL_QUERY_LENGTH
 * @brief The maximum length of a SQL query */
#	define MAX_SQL_QUERY_LENGTH (2048)

/*!
 * @def METADATA_FIELDS_COUNT
 * @brief The number of fields in a package metadata table */
#	define METADATA_FIELDS_COUNT (7)

/*!
 * @def INSTALLATION_DATA_FIELDS_COUNT
 * @brief The number of fields in a package installation data table */
#	define INSTALLATION_DATA_FIELDS_COUNT (8)

/*!
 * @def PRIVATE_FIELDS_COUNT
 * @brief The number fields used internally by the package manager */
#	define PRIVATE_FIELDS_COUNT (1)

/*!
 * @typedef database_type_t
 * @brief A database type */
typedef unsigned int database_type_t;

enum database_types {
	DATABASE_TYPE_METADATA          = 0,
	DATABASE_TYPE_INSTALLATION_DATA = 1,
};

enum package_fields {
	PACKAGE_FIELD_NAME      = 0,
	PACKAGE_FIELD_VERSION   = 1,
	PACKAGE_FIELD_DESC      = 2,
	PACKAGE_FIELD_FILE_NAME = 3,
	PACKAGE_FIELD_ARCH      = 4,
	PACKAGE_FIELD_DEPS      = 5,
	PACKAGE_FIELD_REASON    = 6,
	PACKAGE_FIELD_ID        = 7
};

enum file_fields {
	FILE_FIELD_PACKAGE = 0,
	FILE_FIELD_PATH    = 1,
	FILE_FIELD_ID      = 2
};

/*!
 * @struct package_info_t
 * @brief Package metadata */
typedef struct {
	char *_fields[INSTALLATION_DATA_FIELDS_COUNT]; /*!< The database entry fields */
} package_info_t;

#	define p_name _fields[PACKAGE_FIELD_NAME]
#	define p_version _fields[PACKAGE_FIELD_VERSION]
#	define p_desc _fields[PACKAGE_FIELD_DESC]
#	define p_file_name _fields[PACKAGE_FIELD_FILE_NAME]
#	define p_arch _fields[PACKAGE_FIELD_ARCH]
#	define p_deps _fields[PACKAGE_FIELD_DEPS]
#	define p_reason _fields[PACKAGE_FIELD_REASON]
#	define p_id _fields[PACKAGE_FIELD_ID]

/*!
 * @struct database_t
 * @brief A database */
typedef struct {
	sqlite3 *handle; /*!< A \a SQLite3 handle */
} database_t;

/*!
 * @typedef query_callback_t
 * @brief A callback executed for each database entry returned by a query */
typedef int (*query_callback_t) (void *arg,
                                 int count,
                                 char **values,
                                 char **names);

/*!
 * @fn result_t database_open_read(database_t *database, const char *path)
 * @brief Connects to a read-only database
 * @param database The database
 * @param path The database path
 * @see database_close */
result_t database_open_read(database_t *database, const char *path);

/*!
 * @fn result_t database_open_write(database_t *database,
 *                                  const database_type_t type,
 *                                  const char *path)
 * @brief Connects to a writable database
 * @param database The database
 * @param type The database type
 * @param path The database path
 * @see DATABASE_TYPE_METADATA
 * @see DATABASE_TYPE_INSTALLATION_DATA
 * @see database_close */
result_t database_open_write(database_t *database,
                             const database_type_t type,
                             const char *path);

/*!
 * @fn void database_close(database_t *database)
 * @brief Disconnects from a database
 * @param database The database
 * @see database_open_read
 * @see database_open_write */
void database_close(database_t *database);

/*!
 * @fn result_t database_get(database_t *database,
 *                           const char *name,
 *                           package_info_t *info)
 * @brief Fetches a package (either metadata or installation data) entry from a
 *        database
 * @param database The database
 * @param name The package name
 * @param info The package entry */
result_t database_get(database_t *database,
                      const char *name,
                      package_info_t *info);

/*!
 * @def database_get_metadata
 * @brief Currently, a synonym for database_get. */
#	define database_get_metadata database_get

/*!
 * @def database_get_installation_data
 * @brief Currently, a synonym for database_get. */
#	define database_get_installation_data database_get

/*!
 * @fn void package_info_free(package_info_t *info)
 * @brief Frees all memory consumed by a package entry
 * @param info The package entry */
void package_info_free(package_info_t *info);

/*!
 * @fn result_t database_set_metadata(database_t *database,
 *                                    const package_info_t *info)
 * @brief Adds a package metadata entry to a database
 * @param database The database
 * @param info The package entry */
result_t database_set_metadata(database_t *database,
                               const package_info_t *info);

/*!
 * @fn result_t database_set_installation_data(database_t *database,
 *                                             const package_info_t *info)
 * @brief Adds a package installation data entry to a database
 * @param database The database
 * @param info The package entry */
result_t database_set_installation_data(database_t *database,
                                        const package_info_t *info);

/*!
 * @fn result_t database_remove_installation_data(database_t *database,
 *                                                const char *package)
 * @brief Removes a package installation data entry from a database
 * @param database The database
 * @param package The package name */
result_t database_remove_installation_data(database_t *database,
                                           const char *package);

/*!
 * @fn result_t database_register_path(database_t *database,
 *                                     const char *path,
 *                                     const char *package)
 * @brief Associates a file with an installed package
 * @param database The database
 * @param path The file path
 * @param package The package name */
result_t database_register_path(database_t *database,
                                const char *path,
                                const char *package);

/*!
 * @fn result_t database_unregister_path(database_t *database, const char *path)
 * @brief Unassociates a file with an installed package
 * @param database The database
 * @param path The file path
 * @param package The package name */
result_t database_unregister_path(database_t *database, const char *path);

/*!
 * @fn result_t database_for_each_inst_package(database_t *database,
 *                                             const query_callback_t callback,
 *                                             void *arg)
 * @brief Runs a callback for each installed package
 * @param database The database
 * @param callback The callback to run
 * @param arg A pointer passed to the callback */
result_t database_for_each_inst_package(database_t *database,
                                        const query_callback_t callback,
                                        void *arg);

/*!
 * @def database_for_each_avail_package
 * @brief Currently, a synonym for database_for_each_inst_package. */
#	define database_for_each_avail_package database_for_each_inst_package

/*!
 * @fn result_t database_for_each_file(database_t *database,
 *                                     const char *name,
 *                                     const query_callback_t callback,
 *                                     void *arg)
 * @brief Runs a callback for each file associated with an installed package
 * @param database The database
 * @param name The package name
 * @param callback The callback to run
 * @param arg A pointer passed to the callback */
result_t database_for_each_file(database_t *database,
                                const char *name,
                                const query_callback_t callback,
                                void *arg);

/*!
 * @} */

#endif