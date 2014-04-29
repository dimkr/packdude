#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <dirent.h>
#include "metadata.h"
#include "package.h"
#include "manager.h"
#include "log.h"

result_t manager_open(manager_t *manager, const char *prefix) {
	/* the return value */
	result_t result;

	/* open the package database */
	result = database_open(&manager->database);
	if (RESULT_OK != result)
		goto end;

	/* keep the prefix path */
	manager->prefix = prefix;

	/* report success */
	result = RESULT_OK;

end:
	return result;
}

void manager_close(manager_t *manager) {
	/* close the database */
	database_close(&manager->database);
}

result_t manager_install_by_name(manager_t *manager, const char *name) {
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

	/* if the package is already installed (i.e a recursive dependency), report success */
	if (RESULT_YES == package_is_installed(name, manager->prefix))
		goto end;

	log_write("Querying the package database for %s\n", name);

	/* locate the package */
	result = database_query_by_name(&manager->database, name, &raw_metadata, &size);
	if (RESULT_OK != result)
		goto end;

	/* parse the package metadata */
	result = metadata_parse(raw_metadata, size, &metadata);
	if (RESULT_OK != result)
		goto free_raw_metadata;

	log_write("Getting the dependencies list of %s\n", name);

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

	log_write("Downloading %s (%s)\n", name, file_name);

	/* download the package */
	(void) snprintf((char *) &download_path, sizeof(download_path), "/tmp/%s", file_name);
	result = database_download_by_file_name(&manager->database, file_name, (const char *) &download_path);
	if (RESULT_OK != result)
		goto free_metadata;

	/* open the package */
	log_write("Opening %s\n", file_name);
	result = package_open((const char *) &download_path, &package);
	if (RESULT_OK != result)
		goto free_dependencies;

	/* TODO: delete in case of failure? */

	log_write("Installing the dependencies of %s\n", name);

	/* install the package dependencies */
	dependency = strtok_r(dependencies, " ", &position);
	if (NULL != dependency) {
		do {
			result = manager_install_by_name(manager, dependency);
			switch (result) {
				case RESULT_OK:
				case RESULT_ALREADY_INSTALLED:
					break;
	
				default:	
					goto close_package;
			}
			dependency = strtok_r(NULL, " ", &position);
		} while (NULL != dependency);
	}

	/* TODO: the right installation reason */

	log_write("Installing %s\n", file_name);

	/* install the package */
	result = package_install(&package, INSTALL_REASON_USER, manager->prefix);
	if (RESULT_OK != result)
		goto close_package;

	log_write("Successfully installed %s\n", name);

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

result_t manager_install_by_path(manager_t *manager, const char *path) {
	/* the return value */
	result_t result;

	/* the package */
	package_t package;

	/* open the package */
	result = package_open(path, &package);
	if (RESULT_OK != result)
		goto end;

	/* install the package */
	result = package_install(&package, INSTALL_REASON_USER, manager->prefix);
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

result_t manager_remove(manager_t *manager, const char *name, const bool force) {
	/* the return value */
	result_t result;

	/* remove the package */
	result = package_remove(name, manager->prefix, force);
	if (RESULT_OK != result)
		goto end;

	/* remove unneeded packages */
	result = manager_cleanup(manager);
	if (RESULT_OK != result)
		goto end;
	
	/* report success */
	result = RESULT_OK;

end:
	return result;
}

result_t _cleanup(manager_t *manager) {
	/* the return value */
	result_t result = RESULT_METADATA_DIR_MISSING;

	/* the metadata directory */
	DIR *directory;

	/* a metadata file */
	struct dirent entry;
	struct dirent *name;

	/* the metadata dircetory path */
	char path[PATH_MAX];

	/* open the metadata directory */
	(void) snprintf((char *) &path, sizeof(path), "%s/"PACKAGE_METADATA_DIR, manager->prefix); 
	directory = opendir((const char *) &path);
	if (NULL == directory)
		goto end;

	result = RESULT_NO_UNNEEDED_PACKAGES;
	do {
		/* read the name of one package */
		if (0 != readdir_r(directory, &entry, &name)) {
			result = RESULT_READDIR_R_FAILED;
			goto close_directory;
		}
		if (NULL == name)
			break;
		if (DT_REG != name->d_type)
			continue;

		/* if the package can be removed, try to get rid of it */
		if (RESULT_YES == package_can_remove(name->d_name, manager->prefix)) {
			if (RESULT_OK == package_remove(name->d_name, manager->prefix, false))
				result = RESULT_OK;
		}
	} while (1);

close_directory:
	/* close the metadata directory */
	(void) closedir(directory);

end:
	return result;
}

result_t manager_cleanup(manager_t *manager) {
	result_t result;

	log_write("Looking for unneeded packages\n");

	do {
		result = _cleanup(manager);
		switch (result) {
			case RESULT_OK:
				break;

			case RESULT_NO_UNNEEDED_PACKAGES:
				result = RESULT_OK;

			default:
				goto end;
		}
	} while (1);

end:
	return result;
}
