#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <zlib.h>
#include "../libpackdude/package.h"

int main(int argc, char *argv[]) {
	/* the exit code */
	int exit_code = EXIT_FAILURE;

	/* the input file */
	int input;

	/* the output file */
	int output;

	/* the package metadata file */
	int metadata;
	
	/* the metadata file contents */
	char *metadata_contents;

	/* the archive contents */
	unsigned char *archive;

	/* the package metadata attributes */
	struct stat metadata_attributes;

	/* the archive archive_attributes */
	struct stat archive_attributes;
	
	/* the package header */
	package_header_t header;
	
	/* get the archive size */
	if (-1 == stat(argv[1], &archive_attributes))
		goto end;

	/* get the metadata size */
	if (-1 == stat(argv[2], &metadata_attributes))
		goto end;

	/* open the source file */
	input = open(argv[1], O_RDONLY);
	if (-1 == input)
		goto end;

	/* open the metadata file */
	metadata = open(argv[2], O_RDONLY);
	if (-1 == metadata)
		goto close_input;

	/* open the output file */
	output = open(argv[3], O_CREAT | O_TRUNC | O_WRONLY, 0666);
	if (-1 == output)
		goto close_metadata;

	/* map the metadata to memory */
	metadata_contents = mmap(NULL, (size_t) metadata_attributes.st_size, PROT_READ, MAP_PRIVATE, metadata, 0);
	if (NULL == metadata_contents)
		goto close_output;

	/* map the archive contents to memory */
	archive = mmap(NULL, (size_t) archive_attributes.st_size, PROT_READ, MAP_PRIVATE, input, 0);
	if (NULL == archive)
		goto unmap_metadata;
		
	/* calculate the archive checksum */
	header.checksum = (uint32_t) crc32(crc32(0L, NULL, 0), archive, (uInt) archive_attributes.st_size);

	/* write the package header */
	header.version = 1;
	header.metadata_size = (uint16_t) metadata_attributes.st_size;
	(void) printf("Writing the header (%lu bytes)\n", sizeof(header));
	if (sizeof(header) != write(output, &header, sizeof(header)))
		goto unmap_archive;

	/* write the package the metadata */
	(void) printf("Writing metadata (%zd bytes)\n", metadata_attributes.st_size);
	if ((ssize_t) metadata_attributes.st_size != write(output, metadata_contents, (size_t) metadata_attributes.st_size))
		goto unmap_archive;

	/* write the archive contents */
	(void) printf("Writing the archive (%zd bytes)\n", (size_t) archive_attributes.st_size);
	if ((ssize_t) archive_attributes.st_size != write(output, archive, (size_t) archive_attributes.st_size))
		goto unmap_archive;

	/* report success */
	exit_code = EXIT_SUCCESS;

unmap_archive:
	/* unmap the archive contents */
	(void) munmap(archive, (size_t) archive_attributes.st_size);

unmap_metadata:
	/* unmap the metadata file */
	(void) munmap(metadata_contents, (size_t) metadata_attributes.st_size);

close_output:
	/* close the output file */
	(void) close(output);

close_metadata:
	/* close the metadata file */
	(void) close(metadata);

close_input:
	/* close the input file */
	(void) close(input);

end:	
	return exit_code;
}
