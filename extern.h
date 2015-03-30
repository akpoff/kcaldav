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

enum	type {
	TYPE_CALMULTIGET,
	TYPE_CALQUERY,
	TYPE_MKCALENDAR,
	TYPE_PROPERTYUPDATE,
	TYPE_PROPFIND,
};

enum	calelem {
	CALELEM_CALENDAR_DATA,
	CALELEM_CALENDAR_HOME_SET,
	CALELEM_CALENDAR_MULTIGET,
	CALELEM_CALENDAR_QUERY,
	CALELEM_CALENDAR_USER_ADDRESS_SET,
	CALELEM_CURRENT_USER_PRINCIPAL,
	CALELEM_DISPLAYNAME,
	CALELEM_EMAIL_ADDRESS_SET,
	CALELEM_GETCONTENTTYPE,
	CALELEM_GETETAG,
	CALELEM_HREF,
	CALELEM_MKCALENDAR,
	CALELEM_PRINCIPAL_URL,
	CALELEM_PROP,
	CALELEM_PROPERTYUPDATE,
	CALELEM_PROPFIND,
	CALELEM_RESOURCETYPE,
	CALELEM__MAX
};

enum	proptype {
	PROP_CALENDAR_DATA,
	PROP_CALENDAR_HOME_SET,
	PROP_CALENDAR_USER_ADDRESS_SET,
	PROP_CURRENT_USER_PRINCIPAL,
	PROP_DISPLAYNAME,
	PROP_EMAIL_ADDRESS_SET,
	PROP_GETCONTENTTYPE,
	PROP_GETETAG,
	PROP_PRINCIPAL_URL,
	PROP_RESOURCETYPE,
	PROP__MAX
};

struct	buf {
	char		*buf;
	size_t		 sz;
	size_t		 max;
};

struct	icalnode {
	char		*name;
	char		*val;
	struct icalnode	*parent;
	struct icalnode	*first;
	struct icalnode	*next;
};

struct	ical {
	char	 	 digest[33];
	struct icalnode	*first;
};

struct	prop {
	enum proptype	 key;
	char		*name;
	char		*xmlns;
};

struct	caldav {
	enum type	  type;
	struct prop	 *props;
	size_t		  propsz;
	char		**hrefs;
	size_t		  hrefsz;
};

/*
 * Parsed HTTP ``Authorization'' header (RFC 2617).
 * We only keep the critical parts required for authentication.
 */
struct	httpauth {
	int		 authorised;
	char		*user;
	char		*uri;
	char		*realm;
	char		*nonce;
	char		*response;
};

struct	prncpl {
	char		*name;
	char		*homedir;
};

struct	config {
	char		 *displayname;
	char		 *emailaddress;
	unsigned int	  perms;
#define	PERMS_NONE	  0x00
#define	PERMS_READ	  0x01
#define	PERMS_WRITE	  0x02
#define	PERMS_DELETE	  0x04
#define	PERMS_ALL	 (0x04|0x02|0x01)
};

__BEGIN_DECLS

typedef void	(*ical_putchar)(int, void *);

void 		 bufappend(struct buf *, const char *, size_t);
void		 bufreset(struct buf *);

int		 ical_merge(struct ical *, const struct ical *);
int		 ical_putfile(const char *, const struct ical *);
struct ical 	*ical_parse(const char *);
struct ical 	*ical_parsefile(const char *);
struct ical 	*ical_parsefile_open(const char *, int *);
int		 ical_parsefile_close(const char *, int);
void		 ical_free(struct ical *);
void		 ical_print(const struct ical *, ical_putchar, void *);
void		 ical_printfile(int, const struct ical *);

struct caldav	*caldav_parsefile(const char *);
struct caldav 	*caldav_parse(const char *, size_t);
void		 caldav_free(struct caldav *);

int		 config_parse(const char *, struct config **, const struct prncpl *);
void		 config_free(struct config *);

int		 prncpl_parse(const char *, 
			const struct httpauth *, struct prncpl **);
void		 prncpl_free(struct prncpl *);

struct httpauth	*httpauth_parse(const char *);
void		 httpauth_free(struct httpauth *);

const enum proptype *calprops;
const enum calelem *calpropelems;
const char *const *calelems;

__END_DECLS

#endif
