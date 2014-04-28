#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include "metadata.h"
#include "package.h"
#include "manager.h"

result_t manager_open(manager_t *manager) {
	/* the return value */
	result_t result;

	/* open the package database */
	result = database_open(&manager->database);
	if (RESULT_OK != result)
		goto end;

	/* report success */
	result = RESULT_OK;

end:
	return result;
}

void manager_close(manager_t *manager) {
	/* close the database */
	database_close(&manager->database);
}

result_t manager_install_by_name(manager_t *manager, const char *name, const char *destination) {
	/* the return value */
	result_t result = RESULT_ALREADY_INSTALLED;
	
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
	if (RESULT_YES == package_is_installed(name, destination))
		goto end;

	printf("Querying the package database for %s\n", name);

	/* locate the package */
	result = database_query_by_name(&manager->database, name, &raw_metadata, &size);
	if (RESULT_OK != result)
		goto end;

	printf("Parsing the metadata of %s\n", name);

	/* parse the package metadata */
	result = metadata_parse(raw_metadata, size, &metadata);
	if (RESULT_OK != result)
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
	result = database_download_by_file_name(&manager->database, file_name, (const char *) &download_path);
	if (RESULT_OK != result)
		goto free_metadata;

	/* open the package */
	result = package_open((const char *) &download_path, &package);
	if (RESULT_OK != result)
		goto free_dependencies;

	/* TODO: delete in case of failure? */

	printf("Installing the dependencies of %s\n", name);

	/* install the package dependencies */
	dependency = strtok_r(dependencies, " ", &position);
	if (NULL != dependency) {
		do {
			result = manager_install_by_name(manager, dependency, destination);
			if (RESULT_OK != result)
				goto close_package;
			dependency = strtok_r(NULL, " ", &position);
		} while (NULL != dependency);
	}

	/* TODO: the right installation reason */

	printf("Installing %s\n", name);

	/* install the package */
	result = package_install(&package, INSTALL_REASON_USER, destination);
	if (RESULT_OK != result)
		goto close_package;

	/* report success */
	result = RESULT_OK;

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
	return result;
}

result_t manager_install_by_path(manager_t *manager, const char *path, const char *destination) {
	/* the return value */
	result_t result;
	
	/* the package */
	package_t package;

	/* open the package */
	result = package_open(path, &package);
	if (RESULT_OK != result)
		goto end;

	/* install the package */
	result = package_install(&package, INSTALL_REASON_USER, destination);
	if (RESULT_OK != result)
		goto close_package;

	/* report success */
	result = RESULT_OK;

close_package:
	/* close the package */
	(void) package_close(&package);

end:
	return result;
}
