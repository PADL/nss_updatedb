/*
 * Copyright (c) 2004 PADL Software Pty Ltd.
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

#include "cache.h"

/*
 * Make name service data persistent
 */

struct nss_cache {
	char *filename;
	char *tmpnam;
	void *db;
	int empty;
};

enum nss_status nss_cache_init(const char *filename,
			       nss_cache_t **cache_p)
{
	nss_cache_t *cache;

	cache = (nss_cache_t *)calloc(1, sizeof(*cache));
	if (cache == NULL) {
		return NSS_STATUS_TRYAGAIN;
	}

	cache->filename = NULL;
	cache->tmpnam = NULL;
	cache->db = NULL;
	cache->empty = 1;

	cache->filename = strdup(filename);
	if (cache->filename == NULL) {
		nss_cache_close(&cache);
		return NSS_STATUS_TRYAGAIN;
	}

	*cache_p = cache;

	return NSS_STATUS_SUCCESS;
}

enum nss_status nss_cache_put(nss_cache_t *cache,
			      int where,
			      char *key,
			      char *value)
{
	return NSS_STATUS_SUCCESS;
}

enum nss_status nss_cache_putpwent(nss_cache_t *cache,
				   struct passwd *pw)
{
	if (cache->empty) {
		cache->empty = 0;
	}

	return NSS_STATUS_SUCCESS;
}

enum nss_status nss_cache_putgrent(nss_cache_t *cache,
				   struct group *gr)
{
	if (cache->empty) {
		cache->empty = 0;
	}

	return NSS_STATUS_SUCCESS;
}

enum nss_status nss_cache_commit(nss_cache_t *cache)
{
	if (cache->empty) {
		fprintf(stderr, "Warning: no information was retrieved from the "
			"name service, so the cache will not be replaced\n");
		return NSS_STATUS_UNAVAIL;
	}

	return NSS_STATUS_SUCCESS;
}

enum nss_status nss_cache_close(nss_cache_t **cache_p)
{
	nss_cache_t *cache;

	cache = *cache_p;

	if (cache != NULL) {
		if (cache->filename != NULL)
			free(cache->filename);
		if (cache->tmpnam != NULL)
			free(cache->tmpnam);
		free(cache);
		*cache_p = NULL;
	}

	return NSS_STATUS_SUCCESS;
}

