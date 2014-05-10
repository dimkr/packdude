#include <assert.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>

#include "log.h"
#include "package.h"
#include "manager.h"

result_t manager_new(manager_t *manager, const char *prefix) {
	/* the return value */
	result_t result = RESULT_IO_ERROR;

	assert(NULL != manager);
	assert(NULL != prefix);

	/* change the root directory to the installation prefix */
	log_write(LOG_DEBUG, "Changing the working directory to %s\n", prefix);
	if (-1 == chdir(prefix)) {
		goto end;
	}

	/* open the installation database */
	result = database_open_write(&manager->inst_packages,
	                             DATABASE_TYPE_INSTALLATION_DATA,
	                             INSTALLATION_DATA_DATABASE_PATH);
	if (RESULT_OK != result) {
		log_write(LOG_ERROR, "Failed to open the package database\n");
		goto end;
	}

	/* open the repository */
	result = repo_open(&manager->repo, REPO);
	if (RESULT_OK != result) {
		log_write(LOG_ERROR, "Failed to open the package repository\n");
		goto close_inst;
	}

	/* fetch the repository database */
	result = repo_get_database(&manager->repo, &manager->avail_packages);
	if (RESULT_OK != result) {
		log_write(LOG_ERROR, "Failed to fetch the package database\n");
		goto close_repo;
	}

	/* initialize the installation stack */
	manager->inst_stack = NULL;

	/* report success */
	result = RESULT_OK;
	goto end;

close_repo:
	/* close the repository */
	repo_close(&manager->repo);

close_inst:
	/* close the installed packages database */
	database_close(&manager->inst_packages);

end:
	return result;
}

void manager_free(manager_t *manager) {
	assert(NULL != manager);

	/* close the database */
	database_close(&manager->avail_packages);

	/* close the repository */
	repo_close(&manager->repo);
}

result_t manager_for_each_dependency(manager_t *manager,
                                     const char *name,
                                     const dependency_callback_t callback,
                                     void *arg) {
	/* the package information */
	package_info_t info = {{0}};

	/* the return value */
	result_t result = RESULT_OK;

	/* strtok_r()'s position within the dependencies list */
	char *position = NULL;

	/* a single dependency */
	char *dependency = NULL;

	assert(NULL != manager);
	assert(NULL != name);
	assert(NULL != callback);

	/* get the package metadata */
	result = database_get_metadata(&manager->avail_packages, name, &info);
	if (RESULT_OK != result) {
		goto end;
	}

	/* if the package has no dependencies, do nothing */
	if (0 == strcmp(NO_DEPENDENCIES, info.p_deps)) {
		goto free_metadata;
	}

	/* run the callback for each dependency */
	dependency = strtok_r(info.p_deps, " ", &position);
	while (NULL != dependency) {
		result = callback(dependency, arg);
		if (RESULT_OK != result)
			break;

		/* continue to the next dependency */
		dependency = strtok_r(NULL, " ", &position);
	}

free_metadata:
	/* free the package information */
	package_info_free(&info);

end:
	return result;
}

result_t _install_dependency(const char *name, void *manager) {
	assert(NULL != name);
	assert(NULL != manager);

	return manager_fetch((manager_t *) manager,
	                     name,
	                     INSTALLATION_REASON_DEPENDENCY);
}

result_t manager_is_installed(manager_t *manager, const char *name) {
	/* the package installation data */
	package_info_t installation_data = {{0}};

	/* the return value */
	result_t result = RESULT_NO;

	assert(NULL != manager);
	assert(NULL != name);

	/* check whether the package is already installed */
	log_write(LOG_DEBUG, "Checking whether %s is already installed\n", name);
	result = database_get_installation_data(&manager->inst_packages,
	                                        name,
	                                        &installation_data);
	switch (result) {
		case RESULT_OK:
			log_write(LOG_DEBUG, "%s is installed\n", name);
			package_info_free(&installation_data);
			result = RESULT_YES;
			break;

		case RESULT_NOT_FOUND:
			log_write(LOG_DEBUG, "%s is not installed\n", name);
			result = RESULT_NO;
			break;
	}

	return result;
}

result_t manager_fetch(manager_t *manager,
                       const char *name,
                       const char *reason) {
	/* the package path */
	char path[PATH_MAX] = {'\0'};

	/* the package information */
	package_info_t info = {{0}};

	/* the return value */
	result_t result = RESULT_OK;

	assert(NULL != manager);
	assert(NULL != name);
	assert(NULL != reason);
	assert((0 == strcmp(INSTALLATION_REASON_USER, reason)) ||
	       (0 == strcmp(INSTALLATION_REASON_DEPENDENCY, reason)));

	/* if the package is currently being installed, do nothing */
	if (true == stack_contains(manager->inst_stack,
	                           (node_comparison_t) strcmp,
	                           name)) {
		goto end;
	}

	/* check whether the package is already installed */
	result = manager_is_installed(manager, name);
	switch (result) {
		case RESULT_NO:
			break;

		case RESULT_YES:
			log_write(LOG_WARNING, "%s is already installed; skipping\n", name);
			result = RESULT_OK;
			goto end;

		default:
			log_write(LOG_ERROR,
			          "Failed to determine whether %s is installed\n",
			          name);
			goto end;
	}

	log_write(LOG_DEBUG, "%s is not installed\n", name);

	/* push the package to the installation stack */
	log_write(LOG_DEBUG, "Pushing %s to the installation stack\n", name);
	result = stack_push(&manager->inst_stack, name);
	if (RESULT_OK != result) {
		result = RESULT_MEM_ERROR;
		goto free_info;
	}

	/* get the package metadata */
	package_info_free(&info);
	result = database_get_metadata(&manager->avail_packages, name, &info);
	if (RESULT_OK != result) {
		log_write(LOG_ERROR,
		          "Failed to locate %s in the package database\n",
		          name);
		goto pop_from_stack;
	}

	/* make sure the package is compatible with the architecture the package
	 * manager runs on */
	if ((0 != strcmp(ARCH, info.p_arch)) &&
	    (0 != strcmp(ARCHITECTURE_INDEPENDENT, info.p_arch))) {
		log_write(LOG_ERROR, "The package is incompatible with %s\n", ARCH);
		result = RESULT_INCOMPATIBLE;
		goto pop_from_stack;
	}

	/* fetch the package */
	if (sizeof(path) <= snprintf((char *) &path,
	                             sizeof(path),
	                             "%s/%s",
	                             PACKAGE_ARCHIVE_DIR,
	                             info.p_file_name)) {
		result = RESULT_CORRUPT_DATA;
		goto pop_from_stack;
	}
	log_write(LOG_INFO, "Downloading %s\n", info.p_file_name);
	result = repo_get_package(&manager->repo, &info, (const char *) &path);
	if (RESULT_OK != result) {
		log_write(LOG_ERROR, "Failed to fetch %s\n", name);
		goto pop_from_stack;
	}

	/* verify the package integrity */
	result = package_verify((const char *) &path);
	if (RESULT_OK != result) {
		goto pop_from_stack;
	}

	/* set the package installation reason */
	info.p_reason = strdup(reason);
	if (NULL == info.p_reason) {
		result = RESULT_MEM_ERROR;
		goto pop_from_stack;
	}

	/* install the package dependencies */
	log_write(LOG_DEBUG, "Installing the dependencies of %s\n", name);
	result = manager_for_each_dependency(manager,
	                                     name,
	                                     _install_dependency,
	                                     manager);
	if (RESULT_OK != result) {
		goto pop_from_stack;
	}

	/* install the package itself */
	result = package_install(name,
	                         (const char *) &path,
	                         &manager->inst_packages);
	if (RESULT_OK != result) {
		goto pop_from_stack;
	}

	/* register the package */
	log_write(LOG_INFO, "Registering %s\n", name);
	result = database_set_installation_data(&manager->inst_packages, &info);
	if (RESULT_OK != result) {
		goto pop_from_stack;
	}

	/* report success */
	log_write(LOG_INFO, "Sucessfully installed %s\n", name);
	result = RESULT_OK;

pop_from_stack:
	/* pop the package from the installation stack */
	log_write(LOG_DEBUG, "Popping %s from the installation stack\n", name);
	stack_pop(&manager->inst_stack);

free_info:
	/* free the package information */
	package_info_free(&info);

end:
	return result;
}

int _depends_on(char *name, int count, char **values, char **names) {
	/* the return value */
	int abort = 0;

	/* the package dependencies */
	char *dependencies = NULL;

	/* strtok_r()'s position within the dependencies list */
	char *position = NULL;

	/* a single dependency */
	char *dependency = NULL;

	assert(NULL != name);
	assert(NULL != values[PACKAGE_FIELD_NAME]);
	assert(NULL != values[PACKAGE_FIELD_DEPS]);

	/* do not check whether the package depends on itself */
	if (0 == strcmp(values[PACKAGE_FIELD_NAME], name)) {
		goto end;
	}

	log_write(LOG_DEBUG,
	          "Checking whether %s depends on %s\n",
	          values[PACKAGE_FIELD_NAME],
	          name);

	/* duplicate the dependencies list */
	dependencies = strdup(values[PACKAGE_FIELD_DEPS]);
	if (NULL == dependencies) {
		goto end;
	}

	dependency = strtok_r(dependencies, " ", &position);
	while (NULL != dependency) {
		if (0 == strcmp(dependency, values[PACKAGE_FIELD_NAME])) {
			continue;
		}
		if (0 == strcmp(name, dependency)) {
			log_write(LOG_DEBUG,
			          "%s is required by %s\n",
			          name,
			          values[PACKAGE_FIELD_NAME]);
			abort = 1;
			break;
		}
		dependency = strtok_r(NULL, " ", &position);
	}

	/* free the dependencies list */
	free(dependencies);

end:
	return abort;
}

result_t _remove(manager_t *manager, const char *name) {
	/* the return value */
	result_t result = RESULT_OK;

	assert(NULL != manager);
	assert(NULL != name);

	log_write(LOG_INFO, "Removing files installed by %s\n", name);

	/* remove the package */
	result = package_remove(name, &manager->inst_packages);
	if (RESULT_OK != result) {
		goto end;
	}

	/* report success */
	log_write(LOG_INFO, "Successfully removed %s\n", name);
	result = RESULT_OK;

end:
	return result;
}

result_t manager_remove(manager_t *manager, const char *name) {
	/* the return value */
	result_t result = RESULT_OK;

	assert(NULL != manager);
	assert(NULL != name);

	/* make sure the package is installed */
	result = manager_is_installed(manager, name);
	if (RESULT_YES != result) {
		log_write(LOG_ERROR, "Cannot remove %s; it is not installed\n", name);
		goto end;
	}

	/* make sure the package can be removed - if it's not installed, this check
	 * will fail  */
	if (RESULT_YES != manager_can_remove(manager, name)) {
		log_write(LOG_ERROR,
		          "Cannot remove %s because another package depends on it\n",
		          name);
		goto end;
	}

	/* remove the package */
	result = _remove(manager, name);
	if (RESULT_OK != result) {
		goto end;
	}

	/* report success */
	result = RESULT_OK;

end:
	return result;
}

result_t manager_can_remove(manager_t *manager, const char *name) {
	/* the return value */
	result_t result = RESULT_CORRUPT_DATA;

	assert(NULL != manager);
	assert(NULL != name);

	log_write(LOG_DEBUG, "Checking whether %s can be removed\n", name);

	/* check whether another package depends on the removed one */
	log_write(LOG_DEBUG,
	          "Checking whether another package depends on %s\n",
	          name);
	result = database_for_each_inst_package(&manager->inst_packages,
	                                        (query_callback_t) _depends_on,
	                                        (char *) name);
	switch (result) {
		case RESULT_ABORTED:
			result = RESULT_NO;
			break;

		case RESULT_OK:
			log_write(LOG_DEBUG, "%s is no longer needed\n", name);
			result = RESULT_YES;
			break;
	}

	return result;
}

int _remove_unneeded(manager_cleanup_params_t *params,
                     int count,
                     char **values,
                     char **names) {
	/* the package installation data */
	package_info_t installation_data = {{0}};

	/* the return value */
	int abort = 0;

	assert(NULL != params);
	assert(NULL != params->manager);
	assert(NULL != values[PACKAGE_FIELD_NAME]);

	log_write(LOG_DEBUG,
	          "Attempting to auto-remove %s\n",
	          values[PACKAGE_FIELD_NAME]);

	/* get the package installation data */
	if (RESULT_OK != database_get_installation_data(
		                                        &params->manager->inst_packages,
	                                            values[PACKAGE_FIELD_NAME],
	                                            &installation_data)) {
		abort = 1;
		goto end;
	}

	/* if the package is a dependency, it cannot be cleaned up automatically -
	 * otherwise, the user's applications will just disappear */
	if (0 != strcmp(installation_data.p_reason,
	                INSTALLATION_REASON_DEPENDENCY)) {
		log_write(LOG_DEBUG,
		          "%s cannot be removed because it was installed by the user\n",
		          values[PACKAGE_FIELD_NAME]);
		goto free_installation_data;
	}

	/* remove all uneeded packages */
	if (RESULT_YES == manager_can_remove(params->manager,
	                                     values[PACKAGE_FIELD_NAME])) {
		if (RESULT_OK == _remove(params->manager, values[PACKAGE_FIELD_NAME])) {
			++(params->removed);
		} else {
			log_write(LOG_ERROR,
			          "Failed to auto-remove %s\n",
			          values[PACKAGE_FIELD_NAME]);
			abort = 1;
		}
	} else {
		log_write(LOG_DEBUG,
		          "%s cannot be auto-removed\n",
		          values[PACKAGE_FIELD_NAME]);
	}

free_installation_data:
	/* free the package installation data */
	package_info_free(&installation_data);

end:
	return abort;
}

result_t manager_cleanup(manager_t *manager) {
	/* the callback parameters */
	manager_cleanup_params_t params = {0};

	/* the return value */
	result_t result = RESULT_OK;

	assert(NULL != manager);

	log_write(LOG_INFO, "Cleaning up unneeded packages\n");

	/* remove unneeded packages again and again, until no uneeded packages are
	 * found */
	do {
		params.manager = manager;
		params.removed = 0;
		result = database_for_each_inst_package(
			                                &manager->inst_packages,
	                                        (query_callback_t) _remove_unneeded,
	                                        &params);
		if (RESULT_OK != result) {
			break;
		}
		log_write(LOG_DEBUG, "Removed %u packages\n", params.removed);
	} while (0 < params.removed);

	return result;
}

int _list_package(manager_t *manager,
                  int count,
                  char **values,
                  char **names) {
	assert((METADATA_FIELDS_COUNT == count) ||
	       (INSTALLATION_DATA_FIELDS_COUNT == count));
	assert(NULL != manager);
	assert(NULL != values[PACKAGE_FIELD_NAME]);
	assert(NULL != values[PACKAGE_FIELD_VERSION]);

	log_write(LOG_INFO,
	          "%s %s\n",
	          values[PACKAGE_FIELD_NAME],
	          values[PACKAGE_FIELD_VERSION]);

	return 0;
}

int _list_avail_package(manager_t *manager,
                       int count,
                       char **values,
                       char **names) {
	assert((METADATA_FIELDS_COUNT == count) ||
	       (INSTALLATION_DATA_FIELDS_COUNT == count));
	assert(NULL != manager);
	assert(NULL != values[PACKAGE_FIELD_NAME]);
	assert(NULL != values[PACKAGE_FIELD_VERSION]);

	switch (manager_is_installed(manager, values[PACKAGE_FIELD_NAME])) {
		case RESULT_NO:
			return _list_package(manager, count, values, names);

		case RESULT_YES:
			return 0;

		default:
			return 1;
	}
}

result_t manager_list_inst(manager_t *manager) {
	assert(NULL != manager);

	log_write(LOG_DEBUG, "Listing installed packages");
	return database_for_each_inst_package(&manager->inst_packages,
	                                      (query_callback_t) _list_package,
	                                      manager);
}

result_t manager_list_avail(manager_t *manager) {
	assert(NULL != manager);

	log_write(LOG_DEBUG, "Listing available packages");
	return database_for_each_avail_package(
		                                 &manager->avail_packages,
	                                     (query_callback_t) _list_avail_package,
	                                     manager);
}
