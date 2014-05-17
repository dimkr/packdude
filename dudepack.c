#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include <zlib.h>

#include "package.h"
#include "log.h"

#	define CHUNK_SIZE (BUFSIZ)

int main(int argc, char *argv[]) {
	/* the reading buffer */
	char chunk[CHUNK_SIZE] = {'\0'};

	/* the package header */
	package_header_t header = {0};

	/* the size of a read chunk */
	ssize_t chunk_size = 0;

	/* the total archive size */
	ssize_t total_size = 0;

	/* the exit code */
	int exit_code = EXIT_FAILURE;

	/* make sure no arguments were specified */
	if (1 != argc) {
		log_dump("Usage: dudepack\n");
		goto end;
	}

	/* initialize the checksum */
	header.checksum = (uint32_t) crc32(0L, Z_NULL, 0);

	do {
		/* read a single chunk from standard input */
		chunk_size = read(STDIN_FILENO, (void *) &chunk, sizeof(chunk));
		total_size += chunk_size;
		switch (chunk_size) {
			case 0:
				if (0 == total_size) {
					goto end;
				}
				goto write_header;

			case (-1):
				goto end;

			default:
				/* update the checksum */
				header.checksum = (uint32_t) crc32((uLong) header.checksum,
				                                   (const Bytef *) &chunk,
				                                   (uInt) chunk_size);

				/* write the chunk to standard output */
				if (chunk_size != write(STDOUT_FILENO,
				                        &chunk,
				                        (size_t) chunk_size)) {
					goto end;
				}
		}
	} while (1);

write_header:
	/* write the package header */
	header.magic = MAGIC;
	header.version = VERSION;
	if (sizeof(header) != write(STDOUT_FILENO, &header, sizeof(header))) {
		goto end;
	}

	/* report success */
	exit_code = EXIT_SUCCESS;

end:
	return exit_code;
}