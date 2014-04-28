#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include "metadata.h"
#include "package.h"
#include "manager.h"

bool manager_open(manager_t *manager) {
	/* the return value */
	bool is_success = false;

	/* open the package database */
	if (false == database_open(&manager->database))
		goto end;

	/* report success */
	is_success = true;

end:
	return is_success;
}

void manager_close(manager_t *manager) {
	/* close the database */
	database_close(&manager->database);
}

bool manager_install_by_name(manager_t *manager, const char *name, const char *destination) {
	/* the return value */
	bool is_success = false;
	
	/* the raw package metadata */
	char *raw_metadata;

	/* the metadata size */
	size_t size;

	/* the metadata */
	metadata_t metadata;

	/* the package dependencies */
	char *dependencies;

	/* a single dependency */
	char *dependency;

	/* the package file name */
	const char *file_name;

	/* the package download path */
	char download_path[PATH_MAX];

	/* the downloaded package */
	package_t package;

	/* strtok_r()'s position within the dependencies list */
	char *position;

	printf("Checking whether %s is already installed\n", name);

	/* if the package is already installed (i.e a recursive dependency), report success */
	if (true == package_is_installed(name, destination)) {
		is_success = true;
		goto end;
	}

	printf("Querying the package database for %s\n", name);

	/* locate the package */
	if (false == database_query_by_name(&manager->database, name, &raw_metadata, &size))
		goto end;

	printf("Parsing the metadata of %s\n", name);

	/* parse the package metadata */
	if (false == metadata_parse(raw_metadata, size, &metadata))
		goto free_raw_metadata;

	printf("Getting the dependencies list of %s\n", name);

	/* get the package file name */
	file_name = metadata_get(&metadata, "file_name");
	if (NULL == file_name)
		goto free_metadata;

	/* get the package dependencies list */
	dependencies = (char *) metadata_get(&metadata, "deps");
	if (NULL == dependencies)
		goto free_metadata;
	dependencies = strdup(dependencies);
	if (NULL == dependencies)
		goto free_metadata;

	printf("Downloading %s\n", file_name);

	/* download the package */
	(void) snprintf((char *) &download_path, sizeof(download_path), "/tmp/%s", file_name);
	if (false == database_download_by_file_name(&manager->database, file_name, (const char *) &download_path))
		goto free_metadata;

	/* open the package */
	if (false == package_open((const char *) &download_path, &package))
		goto free_dependencies;

	/* TODO: delete in case of failure? */

	printf("Installing the dependencies of %s\n", name);

	/* install the package dependencies */
	dependency = strtok_r(dependencies, " ", &position);
	if (NULL != dependency) {
		do {
			if (false == manager_install_by_name(manager, dependency, destination))
				goto close_package;
			dependency = strtok_r(NULL, " ", &position);
		} while (NULL != dependency);
	}

	/* TODO: the right installation reason */

	/* install the package */
	if (false == package_install(&package, INSTALL_REASON_USER, destination))
		goto close_package;

	/* report success */
	is_success = true;

close_package:
	/* close the package */
	package_close(&package);

free_dependencies:
	/* free the dependencies list */
	free(dependencies);

free_metadata:
	/* free the metadata */
	metadata_close(&metadata);

free_raw_metadata:
	/* free the raw metadata */
	free(raw_metadata);

end:
	return is_success;
}

bool manager_install_by_path(manager_t *manager, const char *path, const char *destination) {
	/* the return value */
	bool is_success = false;
	
	/* the package */
	package_t package;

	/* open the package */
	if (false == package_open(path, &package))
		goto end;

	/* install the package */
	if (false == package_install(&package, INSTALL_REASON_USER, destination))
		goto close_package;

	/* report success */
	is_success = true;

close_package:
	/* close the package */
	(void) package_close(&package);

end:
	return is_success;
}
