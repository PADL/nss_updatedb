/*
 * Copyright (c) 2004-2008 PADL Software Pty Ltd.
 * All rights reserved.
 * Use is subject to license.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <pwd.h>
#include <grp.h>

#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#else
#error This system does not appear to support the dlfcn interface.
#endif

#include "updatedb.h"
#include "cache.h"

struct nss_backend_handle {
	char *dbname;
	void *dlhandle;
};

typedef struct nss_vtable {
	enum nss_status (*setent)(void);
	enum nss_status (*getent)(void *, char *, size_t, int *);
	enum nss_status (*endent)(void);
} nss_vtable_t;


enum nss_status nss_backend_open(const char *dbname,
				 nss_backend_handle_t **handle_p)
{
	char libpath[PATH_MAX];
	nss_backend_handle_t *handle;

	*handle_p = NULL;

	snprintf(libpath, sizeof(libpath), "libnss_%s.so.2", dbname);

	handle = (nss_backend_handle_t *)calloc(1, sizeof(*handle));
	if (handle == NULL) {
		fprintf(stderr, "Failed to allocate backend handle: %s\n",
			strerror(errno));
		return NSS_STATUS_TRYAGAIN;
	}

	handle->dbname = strdup(dbname);
	if (handle->dbname == NULL) {
		fprintf(stderr, "Failed to allocate backend handle: %s\n",
			strerror(errno));
		nss_backend_close(&handle);
		return NSS_STATUS_TRYAGAIN;
	}

	handle->dlhandle = dlopen(libpath, RTLD_NOW | RTLD_LOCAL);
	if (handle->dlhandle == NULL) {
		fprintf(stderr, "Failed to load nameservice provider %s: %s\n",
			libpath, dlerror());
		nss_backend_close(&handle);
		return NSS_STATUS_UNAVAIL;
	}

	*handle_p = handle;

	return NSS_STATUS_SUCCESS;
}

static enum nss_status _nss_get_vtable(nss_backend_handle_t *handle,
				       unsigned dbname,
				       nss_vtable_t *vtable)
{
	char function[128];
	char *prefix;

	vtable->setent = NULL;
	vtable->getent = NULL;
	vtable->endent = NULL;

	if (dbname == MAP_PASSWD)
		prefix = "pw";
	else if (dbname == MAP_GROUP)
		prefix = "gr";
	else
		assert(0 && "Unknown map type");

	snprintf(function, sizeof(function), "_nss_%s_set%sent",
		 handle->dbname, prefix);
	vtable->setent = dlsym(handle->dlhandle, function);
	if (vtable->setent == NULL) {
		fprintf(stderr, "Failed to find symbol %s: %s\n",
			function, dlerror());
		return NSS_STATUS_UNAVAIL;
	}

	snprintf(function, sizeof(function), "_nss_%s_get%sent_r",
		 handle->dbname, prefix);
	vtable->getent = dlsym(handle->dlhandle, function);
	if (vtable->getent == NULL) {
		fprintf(stderr, "Failed to find symbol %s: %s\n",
			function, dlerror());
		return NSS_STATUS_UNAVAIL;
	}

	snprintf(function, sizeof(function), "_nss_%s_end%sent",
		 handle->dbname, prefix);
	vtable->endent = dlsym(handle->dlhandle, function);
	if (vtable->endent == NULL) {
		fprintf(stderr, "Failed to find symbol %s: %s\n",
			function, dlerror());
		return NSS_STATUS_UNAVAIL;
	}

	return NSS_STATUS_SUCCESS;
}

static enum nss_status _nss_pw_callback(nss_backend_handle_t *handle,
					void *result,
					void *private)
{
	return nss_cache_putpwent((nss_cache_t *)private,
				  (struct passwd *)result);
}

static enum nss_status _nss_gr_callback(nss_backend_handle_t *handle,
					void *result,
					void *private)
{
	return nss_cache_putgrent((nss_cache_t *)private,
				  (struct group *)result);
}

static enum nss_status _nss_enumerate(nss_backend_handle_t *handle,
				      nss_vtable_t *vtable,
				      enum nss_status (*callback)(nss_backend_handle_t *, void *, void *),
				      void *private)
{
	char *buffer;
	size_t buflen = 1024;
	enum nss_status status;
	int max_retry = 10;
	int errnop;
	union {
		struct passwd pw;
		struct group gr;
	} result;

	buffer = malloc(buflen);
	if (buffer == NULL) {
		return NSS_STATUS_TRYAGAIN;
	}

	status = vtable->setent();
	if (status != NSS_STATUS_SUCCESS) {
		return status;
	}

tryagain:
	do {
		status = vtable->getent((void *)&result, buffer, buflen, &errnop);
		if (status != NSS_STATUS_SUCCESS) {
			break;
		}
		status = callback(handle, (void *)&result, private);
	} while (status == NSS_STATUS_SUCCESS);

	if (status == NSS_STATUS_TRYAGAIN) {
		char *tmp;

		buflen *= 2;
		tmp = realloc(buffer, buflen);
		if (tmp == NULL) {
			vtable->endent();
			free(buffer);
			return NSS_STATUS_TRYAGAIN;
		}
		buffer = tmp;
		status = NSS_STATUS_SUCCESS; /* enter the loop again */
		max_retry--;
		if (max_retry >= 0)
			goto tryagain;
	}

	vtable->endent();
	
	free(buffer);

	return (status == NSS_STATUS_NOTFOUND) ? NSS_STATUS_SUCCESS : status;
}

enum nss_status nss_update_db(nss_backend_handle_t *handle,
			      unsigned map,
			      const char *filename)
{
	enum nss_status (*callback)(nss_backend_handle_t *, void *, void *);
	nss_vtable_t vtable;
	nss_cache_t *cache;
	enum nss_status status;

	status = _nss_get_vtable(handle, map, &vtable);
	if (status != NSS_STATUS_SUCCESS) {
		return status;
	}

	if (map == MAP_PASSWD)
		callback = _nss_pw_callback;
	else if (map == MAP_GROUP)
		callback = _nss_gr_callback;
	else
		assert(0 && "Unknown map type");

	status = nss_cache_init(filename, &cache);
	if (status != NSS_STATUS_SUCCESS) {
		fprintf(stderr, "Failed to open backing cache in %s: %s\n",
			filename, strerror(errno));
		return status;
	}

	status = _nss_enumerate(handle, &vtable, callback, cache);
	if (status == NSS_STATUS_SUCCESS) {
		status = nss_cache_commit(cache);
	} else {
		fprintf(stderr, "Failed to enumerate nameservice: %s\n", strerror(errno));
		status = nss_cache_abort(cache);
	}

	nss_cache_close(&cache);

	return status;
}

enum nss_status nss_backend_close(nss_backend_handle_t **handle_p)
{
	nss_backend_handle_t *handle;

	handle = *handle_p;

	if (handle != NULL) {
		if (handle->dlhandle != NULL) {
			dlclose(handle->dlhandle);
		}
		if (handle->dbname != NULL) {
			free(handle->dbname);
		}
		free(handle);
		*handle_p = NULL;
	}

	return NSS_STATUS_SUCCESS;
}

