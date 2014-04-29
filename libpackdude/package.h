#ifndef _PACKAGE_H_INCLUDED
#	define _PACKAGE_H_INCLUDED

#	include <stdint.h>
#	include <stdbool.h>
#	include "error.h"
#	include "metadata.h"
#	include "flist.h"
#	include "archive.h"

#	define PACKAGE_METADATA_DIR VAR_DIR"/packdude/metadata"
#	define PACKAGE_CONTENTS_DIR VAR_DIR"/packdude/contents"

typedef char *install_reason_t;

#	define INSTALL_REASON_USER "user"
#	define INSTALL_REASON_DEPENDENCY "dependency"
#	define INSTALL_REASON_SYSTEM "system"

typedef struct __attribute__((packed)) {
	uint8_t version;
	uint32_t checksum;
	uint16_t metadata_size;
} package_header_t;

typedef struct {
	int fd;
	const package_header_t *header;
	metadata_t metadata;
	unsigned char *contents;
	off_t size;
	archive_t archive;
	file_list_t file_list;
} package_t;

result_t package_open(const char *path, package_t *package);
result_t package_can_install(const package_t *package, const char *destination);
result_t package_can_remove(const char *name, const char *source);
result_t package_install(package_t *package, const install_reason_t reason, const char *destination);
result_t package_remove(const char *name, const char *source, const bool force);
void package_close(package_t *package);

result_t package_is_installed(const char *name, const char *destination);

#endif
