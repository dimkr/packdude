#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <zlib.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include "package.h"
#include "log.h"

result_t package_open(const char *path, package_t *package) {
	/* the return value */
	result_t result = RESULT_FILE_MISSING;

	/* the package attributes */
	struct stat attributes;

	/* the raw package metadata */
	const char *raw_metadata;

	/* the archive contained inside the package */
	unsigned char *archive;

	/* the archive size */
	uInt archive_size;

	/* the package architecture */
	const char *architecture;

	/* get the package size */
	if (-1 == stat(path, &attributes))
		goto end;

	/* make sure the package size is at least the size of the header, 1 byte of metadata and a 1-byte archive */
	if ((sizeof(package_header_t) + 2) > attributes.st_size)
		goto end;

	/* open the package */
	package->fd = open(path, O_RDONLY);
	if (-1 == package->fd)
		goto end;

	/* map the archive contents to memory */
	package->contents = mmap(NULL, (size_t) attributes.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, package->fd, 0);
	if (NULL == package->contents) {
		result = RESULT_MMAP_FAILED;
		goto close_package;
	}

	/* locate the package header */
	package->header = (const package_header_t *) package->contents;

	/* locate the package metadata */
	raw_metadata = (const char *) package->contents + sizeof(package_header_t);

	/* make sure the metadata size is at least the size of "a=b", the smallest possible metadata */
	if ((3 * sizeof(char)) > package->header->metadata_size) {
		result = RESULT_INVALID_SIZE;
		goto unmap_contents;
	}

	/* locate the archive contained in the package */
 	archive = (unsigned char *) package->header + sizeof(package_header_t) + package->header->metadata_size;

	/* make sure the archive is not empty */
	archive_size = ((uInt) attributes.st_size) - (archive - package->contents);
	if (0 == archive_size) {
		result = RESULT_INVALID_SIZE;
		goto unmap_contents;
	}

	/* parse the package metadata */
	result = metadata_parse(raw_metadata, (size_t) package->header->metadata_size, &package->metadata);
	if (RESULT_OK != result)
		goto unmap_contents;

	/* get the package architecture */
	architecture = metadata_get(&package->metadata, "arch");
	if (NULL == architecture) {
		result = RESULT_NO_ARCHITECURE;
		goto unmap_contents;
	}

	/* check whether the package is incompatible */
	if ((0 != strcmp(ARCH, architecture)) && (0 != strcmp("all", architecture))) {
		result = RESULT_INCOMPATIBLE_ARCHITECURE;
		goto free_metadata;
	}

	/* verify the package checksum */
	if (((uInt) package->header->checksum) != crc32(crc32(0L, NULL, 0), archive, archive_size)) {
		result = RESULT_BAD_CHECKSUM;
		goto free_metadata;
	}

	/* open the archive */
	result = archive_open(archive, (size_t) archive_size, &package->archive);
	if (RESULT_OK != result)
		goto free_metadata;

	/* save the package size */
	package->size = attributes.st_size;

	/* report success */
	result = RESULT_OK;
	goto end;

free_metadata:
	/* free the package metadata */
	metadata_close(&package->metadata);

unmap_contents:
	/* unmap the package contents */
	(void) munmap(package->contents, (size_t) attributes.st_size);

close_package:
	/* close the package */
	(void) close(package->fd);

end:
	return result;
}

void package_close(package_t *package) {
	/* close the archive */
	archive_close(&package->archive);

	/* free the package metadata */
	metadata_close(&package->metadata);

	/* unmap the package contents */
	(void) munmap(package->contents, package->size);

	/* close the file */
	(void) close(package->fd);
}

result_t package_can_install(const package_t *package, const char *destination) {
	/* the return value */
	result_t result = RESULT_NO_DEPENDENCIES_FIELD;

	/* the package dependencies */
	char *dependencies;

	/* a single dependency */
	char *dependency;

	/* strtok_r()'s position within the dependencies list */
	char *position;

	/* get the package's dependencies list */
	dependencies = (char *) metadata_get(&package->metadata, "deps");
	if (NULL == dependencies)
		goto end;

	/* make sure all dependencies are installed */
	if (0 == strlen(dependencies))
		dependencies = NULL;
	else {
		dependencies = strdup(dependencies);
		if (NULL == dependencies) {
			result = RESULT_STRDUP_FAILED;
			goto end;
		}
		dependency = strtok_r(dependencies, " ", &position);
		if (NULL == dependency) {
			result = RESULT_INVALID_DEPENDENCIES;
			goto free_dependencies;
		}
		do {
			if (RESULT_NO == package_is_installed(dependency, destination))
				goto free_dependencies;
			dependency = strtok_r(NULL, " ", &position);
		} while (NULL != dependency);
	}

	/* report the package can be installed */
	result = RESULT_OK;

free_dependencies:
	/* free the dependencies list */
	if (NULL != dependencies)
		free(dependencies);

end:
	return result;
}

result_t package_install(package_t *package,
                     const install_reason_t reason,
                     const char *destination) {
	/* the return value */
	result_t result = RESULT_NO_NAME_FIELD;

	/* the package name */
	const char *name;

	/* the package metadata path */
	char metadata[PATH_MAX];

	/* the package file list path */
	char file_list_path[PATH_MAX];

	/* the package file list */
	FILE *file_list;

	/* a loop index */
	int i;

	/* get the package name */
	name = metadata_get(&package->metadata, "name");
	if (NULL == name)
		goto end;

	/* check whether the package can be installed */
	result = package_can_install(package, destination);
	if (RESULT_OK != result)
		goto end;

	/* add a metadata field which specifies the installation reason */
	result = metadata_add(&package->metadata, "reason", (const char *) reason);
	if (RESULT_OK != result)
		goto end;

	/* create the package file list */
	(void) snprintf((char *) &file_list_path, sizeof(file_list_path), "%s/"PACKAGE_CONTENTS_DIR"/%s", destination, name);
	file_list = fopen((const char *) &file_list_path, "w");
	if (NULL == file_list) {
		result = RESULT_FOPEN_FAILED;
		goto end;
	}
	
	/* save a copy of the package metadata */
	(void) snprintf((char *) &metadata, sizeof(metadata), "%s/"PACKAGE_METADATA_DIR"/%s", destination, name);
	result = metadata_dump(&package->metadata, (char *) &metadata);
	if (RESULT_OK != result)
		goto close_file_list;

	/* initialize the package file list */
	file_list_new(&package->file_list);

	/* extract the archive */
	result = archive_extract(&package->archive, &package->file_list, destination);
	if (RESULT_OK != result)
		goto free_file_list;

	/* write the paths of all extracted files to the file list, in reversed order */
	for (i = (int) package->file_list.count - 1; 0 <= i; --i) {
		if (0 > fprintf(file_list, "%s\n", package->file_list.paths[i])) {
			result = RESULT_FPRINTF_FAILED;
			goto free_file_list;
		}
	}

	/* report success */
	result = RESULT_OK;

free_file_list:
	/* free the package file list */
	file_list_free(&package->file_list);

close_file_list:
	/* close the file list */
	(void) fclose(file_list);

end:
	return result;
}

result_t package_is_installed(const char *name, const char *destination) {
	/* the return value */
	result_t is_installed = RESULT_NO;

        /* the package metadata path */
        char metadata[PATH_MAX];

	/* the metadata attributes */
	struct stat attributes;

	/* check whether the package metadata exists */
	(void) snprintf((char *) &metadata, sizeof(metadata), "%s/"PACKAGE_METADATA_DIR"/%s", destination, name);
	if (-1 == stat((const char *) &metadata, &attributes))
		goto end;

	/* report the package is installed */
	is_installed = RESULT_YES;

end:
	return is_installed;
}

result_t package_can_remove(const char *name, const char *source) {
	/* the return value */
	result_t result = RESULT_GETCWD_FAILED;

	/* the metadata directory */
	DIR *directory;

	/* a metadata file */
	struct dirent entry;
	struct dirent *raw_metadata;

	/* parsed metadata */
	metadata_t metadata;

	/* the working directory */
	char working_directory[PATH_MAX];

	/* the metadata dircetory */
	char metadata_directory[PATH_MAX];

	/* package dependencies */
	char *dependencies;

	/* a single dependency */
	char *dependency;

	/* strtok_r()'s position within the dependencies list */
	char *position;

        /* get the working directory path */
        if (NULL == getcwd((char *) &working_directory, sizeof(working_directory)))
                goto end;

	/* change the working directory to the metadata directory */
	(void) snprintf((char *) &metadata_directory, sizeof(metadata_directory), "%s/"PACKAGE_METADATA_DIR, source);
	if (-1 == chdir((const char *) &metadata_directory)) {
		result = RESULT_CHDIR_FAILED;
		goto end;
	}

	/* open the metadata directory */
	directory = opendir(".");
	if (NULL == directory) {
		result = RESULT_METADATA_DIR_MISSING;
		goto restore_working_directory;
	}

	result = RESULT_OK;
	do {
		/* read the name of one metadata file */
		if (0 != readdir_r(directory, &entry, &raw_metadata)) {
			result = RESULT_READDIR_R_FAILED;
			goto close_directory;
		}
		if (NULL == raw_metadata)
			break;
		if (DT_REG != raw_metadata->d_type)
			continue;

		/* parse the metadata file */
		result = metadata_parse_file(raw_metadata->d_name, &metadata);
		if (RESULT_OK != result)
			break;

		/* obtain the list of dependencies */
		dependencies = (char *) metadata_get(&metadata, "deps");
		if (NULL == dependencies) {
			result = RESULT_CORRUPT_METADATA;
			goto free_metadata;
		}
		if (0 == strlen(dependencies))
			goto free_metadata;
		dependencies = strdup(dependencies);
		if (NULL == dependencies) {
			result = RESULT_STRDUP_FAILED;
			goto free_metadata;
		}

		/* check the package depends on the removed package */
		dependency = strtok_r(dependencies, " ", &position);
		if (NULL == dependency) {
			result = RESULT_INVALID_DEPENDENCIES;
			goto free_dependencies;
		}
		do {
			if (0 == strcmp(name, dependency)) {
				result = RESULT_NO;
				break;
			}
			dependency = strtok_r(NULL, " ", &position);
		} while (NULL != dependency);

free_dependencies:
		/* free the dependencies list */
		free(dependencies);

free_metadata:
		/* free the parsed metadata */
		metadata_close(&metadata);

		if (RESULT_OK != result)
			break;
	} while (1);

	if (RESULT_OK == result)
		result = RESULT_YES;

close_directory:
	/* close the metadata directory */
	(void) closedir(directory);

restore_working_directory:
	/* return to the previous working directory */
	(void) chdir((const char *) &working_directory);

end:
	return result;
}

result_t package_remove(const char *name, const char *source, const bool force) {
	/* the return value */
	result_t result;

	/* the package file list path */
	char file_list_path[PATH_MAX];

	/* the package file list */
	FILE *file_list;

	/* a file installed by the package */
	char path[PATH_MAX];

	/* the absolute path of the file */
	char absolute_path[PATH_MAX];

	/* the file attributes */
	struct stat attributes;

	/* check whether the package can be removed */
	if (false == force) {
		result = package_can_remove(name, source);
		if (RESULT_YES != result) {
			log_write("%s cannot be removed.\n", name);
			goto end;
		}
	}

	log_write("Removing %s\n", name);

	/* open the package file list */
	(void) snprintf((char *) &file_list_path, sizeof(file_list_path), "%s/"PACKAGE_CONTENTS_DIR"/%s", source, name);
	file_list = fopen((const char *) &file_list_path, "r");
	if (NULL == file_list) {
		result = RESULT_FOPEN_FAILED;
		goto end;
	}

	do {
		/* read the path of one file */
		if (NULL == fgets((char *) &path, sizeof(path), file_list))
			break;

		/* trim the trailing line break */
		path[strlen((const char *) &path) - 1] = '\0';

		/* get the file attributes */
		(void) snprintf((char *) &absolute_path, sizeof(absolute_path), "%s/%s", source, (const char *) &path);
		if (-1 == lstat((const char *) &absolute_path, &attributes)) {
			if (ENOENT == errno)
				continue;
		}

		/* delete the file */
		if (S_ISDIR(attributes.st_mode)) {
			if (-1 == rmdir((const char *) &absolute_path)) {
				if ((ENOENT != errno) && (ENOTEMPTY != errno)) {
					result = RESULT_RMDIR_FAILED;
					goto close_file_list;
				}
			}
		} else {
			if (-1 == unlink((const char *) &absolute_path)) {
				if (ENOENT != errno) {
					result = RESULT_UNLINK_FAILED;
					goto close_file_list;
				}
			}
		}

		/* write a progress indicator */
		log_write(".");
	} while (1);

	/* end the output with a line break */
	log_write("\n");

	/* delete the package file list */
	if (-1 == unlink((const char *) &file_list_path)) {
		result = RESULT_UNLINK_FAILED;
		goto close_file_list;
	}

	/* delete the package metadata */
	(void) snprintf((char *) &file_list_path, sizeof(file_list_path), "%s/"PACKAGE_METADATA_DIR"/%s", source, name);
	if (-1 == unlink((const char *) &file_list_path)) {
		result = RESULT_UNLINK_FAILED;
		goto close_file_list;
	}

	log_write("Successfully removed %s\n", name);

	/* report success */
	result = RESULT_OK;

close_file_list:
	/* close the file list */
	(void) fclose(file_list);

end:
	return result;
}
