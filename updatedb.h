/*
 * Copyright (c) 2004 PADL Software Pty Ltd.
 * All rights reserved.
 * Use is subject to license.
 */

#ifndef _NSS_UPDATEDB_H_
#define _NSS_UPDATEDB_H_ 1

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_NSS_H
#include <nss.h>
#else
#error This system does not appear to support the GNU NSS interface.
#endif

#define MAP_PASSWD	0x01
#define MAP_GROUP	0x02
#define MAP_ALL		(MAP_PASSWD | MAP_GROUP)

#ifndef DB_DIR
#define DB_DIR		"/var/db"
#endif

#define DB_PASSWD	DB_DIR "/passwd.db"
#define DB_GROUP	DB_DIR "/group.db"

struct nss_backend_handle;
typedef struct nss_backend_handle nss_backend_handle_t;
	
enum nss_status nss_backend_open(const char *dbname,
				 nss_backend_handle_t **handle);

enum nss_status nss_update_db(nss_backend_handle_t *handle,
			      unsigned dbname,
			      const char *filename);

enum nss_status nss_backend_close(nss_backend_handle_t **handle);

#endif /* _NSS_UPDATEDB_H_ */

