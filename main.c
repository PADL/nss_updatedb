/*
 * Copyright (c) 2004 PADL Software Pty Ltd.
 * All rights reserved.
 * Use is subject to license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#include "updatedb.h"

static void usage(void)
{
	fprintf(stderr, "Usage: nss_updatedb [nameservice] [passwd|group]\n");
	exit(1);
}

static void print_nss_error(const char *msg, enum nss_status status)
{
	const char *serr;

	assert(status != NSS_STATUS_SUCCESS);

	switch (status) {
	case NSS_STATUS_TRYAGAIN:
		serr = "out of memory";
		break;
	case NSS_STATUS_UNAVAIL:
		serr = "name service unavailable";
		break;
	case NSS_STATUS_NOTFOUND:
		serr = "not found";
		break;
	default:
		serr = "unknown error";
		break;
	}

	fprintf(stderr, "%s: %s\n", msg, serr);
}

int main(int argc, char *argv[])
{
	nss_backend_handle_t *handle;
	unsigned maps = 0;
	char *dbname;
	enum nss_status status;

	if (argc < 2 || argc > 3) {
		usage();
	}

	dbname = argv[1];

	if (argc == 3) {
		char *mapname;

		mapname = argv[2];

		if (strcmp(mapname, "passwd") == 0)
			maps = MAP_PASSWD;
		else if (strcmp(mapname, "group") == 0)
			maps = MAP_GROUP;
		else
			usage();
	} else {
		maps = MAP_ALL;
	}

	status = nss_backend_open(dbname, &handle);
	if (status != NSS_STATUS_SUCCESS) {
		exit(status);
	}

	if (maps & MAP_PASSWD) {
		status = nss_update_db(handle, MAP_PASSWD, DB_PASSWD);
		if (status != NSS_STATUS_SUCCESS)
			exit(status);
	}

	if (maps & MAP_GROUP) {
		status = nss_update_db(handle, MAP_GROUP, DB_GROUP);
		if (status != NSS_STATUS_SUCCESS)
			exit(status);
	}

	exit(0);
}

