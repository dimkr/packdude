#ifndef _METADATA_H_INCLUDED
#	define _METADATA_H_INCLUDED

#	include <stdbool.h>
#	include <stdint.h>

typedef struct {
	const char *name;
	const char *value;
} metadata_field_t;

typedef struct {
	char *copy;
	metadata_field_t *fields;
	unsigned int count;
} metadata_t;

bool metadata_parse(const char *raw, const size_t size, metadata_t *metadata);
bool metadata_add(metadata_t *metadata, const char *name, const char *value);
const char *metadata_get(const metadata_t *metadata, const char *name);
void metadata_close(metadata_t *metadata);
bool metadata_dump(const metadata_t *metadata, const char *path);

#endif
