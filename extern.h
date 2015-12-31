/*	$Id$ */
/*
 * Copyright (c) 2015 Kristaps Dzonsons <kristaps@bsd.lv>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#ifndef EXTERN_H
#define EXTERN_H

#include "libkcaldav.h"

#define	KREALM		"kcaldav"

/*
 * Resource in a calendar collection.
 */
struct	res {
	char		*data;
	struct ical	*ical;
	int64_t		 etag;
	char		*url;
	int64_t		 collection;
	int64_t		 id;
};

/*
 * A calendar collection.
 */
struct	coln {
	char		*url; /* name of collection */
	char		*displayname; /* displayname of collection */
	char		*colour; /* colour (RGBA) */
	char		*description; /* free-form description */
	int64_t		 ctag; /* collection tag */
	int64_t		 id; /* unique identifier */
};

/*
 * Proxies for a principal.
 */
struct	proxy {
	int64_t		 proxy;
	char		*email;
	char		*name;
	int64_t		 bits;
	int64_t		 id;
};

/*
 * A principal is a user of the system.
 */
struct	prncpl {
	char		*name; /* username */
	char		*hash; /* MD5 of name, realm, and password */
	uint64_t	 quota_used; /* quota (VFS) */
	uint64_t	 quota_avail; /* quota (VFS) */
	char		*email; /* email address */
	struct coln	*cols; /* owned collections */
	size_t		 colsz; /* number of owned collections */
	struct proxy	*proxies;
	size_t		 proxiesz;
	int64_t		 id; /* unique identifier */
};

/*
 * Return codes for nonce operations.
 */
enum	nonceerr {
	NONCE_ERR, /* generic error */
	NONCE_NOTFOUND, /* nonce entry not found */
	NONCE_REPLAY, /* replay attack detected! */
	NONCE_OK /* nonce checks out */
};

__BEGIN_DECLS

void		  db_collection_free(struct coln *);
int		  db_collection_load(struct coln **, const char *, int64_t);
int		  db_collection_loadid(struct coln **, int64_t, int64_t);
int		  db_collection_new(const char *, const struct prncpl *);
int		  db_collection_remove(int64_t, const struct prncpl *);
int		  db_collection_resources(void (*)(const struct res *, void *), int64_t, void *);
int		  db_collection_update(const struct coln *, const struct prncpl *);
int		  db_init(const char *, int);
int		  db_nonce_new(char **);
enum nonceerr	  db_nonce_update(const char *, size_t);
enum nonceerr	  db_nonce_validate(const char *, size_t);
int		  db_owner_check_or_set(int64_t);
int64_t		  db_prncpl_identify(const char *);
int		  db_prncpl_load(struct prncpl **, const char *);
int		  db_prncpl_new(const char *, const char *, const char *, const char *);
int		  db_prncpl_proxies(const struct prncpl *, void (*)(const char *, int64_t, void *), void *);
int		  db_prncpl_update(const struct prncpl *);
int		  db_proxy(const struct prncpl *, int64_t, int64_t);
int		  db_resource_delete(const char *, int64_t, int64_t);
int		  db_resource_remove(const char *, int64_t);
int		  db_resource_load(struct res **, const char *, int64_t);
int		  db_resource_new(const char *, const char *, int64_t);
int		  db_resource_update(const char *, const char *, int64_t, int64_t);

void		  prncpl_free(struct prncpl *);

void		  res_free(struct res *);

__END_DECLS

#endif
