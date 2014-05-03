#ifndef _PACKAGE_H_INCLUDED
#	define _PACKAGE_H_INCLUDED

#	include <stdint.h>
#	include "result.h"
#	include "database.h"

typedef struct __attribute__((packed)) {
	uint8_t version;
	uint32_t checksum;
} package_header_t;

typedef struct {
	database_t *database;
	const char *package;
} file_register_params_t;

result_t package_verify(const char *path);
result_t package_install(const char *name,
                         const char *path,
                         database_t *database);
result_t package_remove(const char *name, database_t *database);

#endif