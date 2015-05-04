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
#ifndef KCALDAV_H
#define KCALDAV_H

struct	state {
	struct prncpl	*prncpl; /* current user principal */
	struct coln	*cfg; /* (resource in?) requested collection */
	char		 caldir[PATH_MAX]; /* calendar root */
	char		*principal; /* principal in request */
	char		*collection; /* collection in request */
	char		*resource; /* resource in request */
};

enum	xml {
	XML_CALDAV_CALENDAR,
	XML_CALDAV_CALENDAR_DATA,
	XML_CALDAV_COMP,
	XML_CALDAV_OPAQUE,
	XML_DAV_BIND,
	XML_DAV_COLLECTION,
	XML_DAV_HREF,
	XML_DAV_MULTISTATUS,
	XML_DAV_PRIVILEGE,
	XML_DAV_PROP,
	XML_DAV_PROPSTAT,
	XML_DAV_READ,
	XML_DAV_READ_CURRENT_USER_PRIVILEGE_SET,
	XML_DAV_RESPONSE,
	XML_DAV_STATUS,
	XML_DAV_UNBIND,
	XML_DAV_WRITE,
	XML__MAX
};

enum	page {
	PAGE_INDEX = 0,
	PAGE_SETEMAIL,
	PAGE_SETPASS,
	PAGE__MAX
};

enum	valid {
	VALID_BODY = 0,
	VALID_COLOUR,
	VALID_DESCRIPTION,
	VALID_EMAIL,
	VALID_NAME,
	VALID_PASS,
	VALID_URI,
	VALID__MAX
};

typedef	 void (*collectionfp)(struct kxmlreq *, 
		const struct coln *);
typedef	 void (*resourcefp)(struct kxmlreq *, 
		const struct coln *, const struct res *);

/*
 * This fully describes the properties that we handle and their various
 * callbacks and informational bits.
 */
struct	property {
	unsigned int	flags;
	collectionfp	cgetfp;
	resourcefp	rgetfp;
};

__BEGIN_DECLS

int	 xml_ical_putc(int, void *);
int	 http_ical_putc(int, void *);

void	 http_error(struct kreq *, enum khttp);
int	 http_paths(const char *, char **, char **, char **);

void	 method_delete(struct kreq *);
void	 method_dynamic_get(struct kreq *);
void	 method_dynamic_post(struct kreq *);
void	 method_get(struct kreq *);
void	 method_options(struct kreq *);
void	 method_propfind(struct kreq *);
void	 method_proppatch(struct kreq *);
void	 method_put(struct kreq *);
void	 method_report(struct kreq *);

extern const char *const pages[PAGE__MAX];
extern const char *const xmls[XML__MAX];
extern const char *const valids[VALID__MAX];
extern const struct property properties[PROP__MAX];

__END_DECLS

#endif
