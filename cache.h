/*
 * Copyright (c) 2004 PADL Software Pty Ltd.
 * All rights reserved.
 * Use is subject to license.
 */

#ifndef _NSS_CACHE_H_
#define _NSS_CACHE_H_ 1

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_NSS_H
#include <nss.h>
#else
#error This system does not appear to support the GNU NSS interface.
#endif

#include <pwd.h>
#include <grp.h>

struct nss_cache;
typedef struct nss_cache nss_cache_t;

enum nss_status nss_cache_init(const char *filename,
			       nss_cache_t **store_p);

enum nss_status nss_cache_put(nss_cache_t *store,
			      const char *key,
			      const char *value);

enum nss_status nss_cache_putpwent(nss_cache_t *store,
				   struct passwd *pw);

enum nss_status nss_cache_putgrent(nss_cache_t *store,
				   struct group *gr);

enum nss_status nss_cache_commit(nss_cache_t *store);

enum nss_status nss_cache_close(nss_cache_t **store_p);

#endif /* _NSS_CACHE_H_ */

