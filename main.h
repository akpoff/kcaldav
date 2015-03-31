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
#ifndef MAIN_H
#define MAIN_H

/*
 * We carry this state around with us after successfully setting up our
 * environment: HTTP authorised (auth), HTTP authorisation matched with
 * a local principal (prncpl), and configuration file read for the
 * relevant directory (cfg) with permission mapped to the current
 * principal.
 * The "path" component is the subject of the HTTP query mapped to the
 * local CALDIR.
 */
struct	state {
	struct prncpl	*prncpl;
	struct config	*cfg;
	struct httpauth	*auth;
	char		 path[PATH_MAX]; /* filesystem path */
	char		 dir[PATH_MAX]; /* "path" directory part */
	char		 ctagfile[PATH_MAX]; /* ctag filename */
	char		 configfile[PATH_MAX]; /* config filename */
	char		 prncplfile[PATH_MAX]; /* passwd filename */
	int		 isdir;
};

enum	xml {
	XML_CALDAV_CALENDAR,
	XML_DAV_COLLECTION,
	XML_DAV_HREF,
	XML_DAV_MULTISTATUS,
	XML_DAV_PROP,
	XML_DAV_PROPSTAT,
	XML_DAV_RESPONSE,
	XML_DAV_STATUS,
	XML_DAV_UNAUTHENTICATED,
	XML__MAX
};

__BEGIN_DECLS

typedef	 void (*collectionfp)(struct kxmlreq *);
typedef	 void (*resourcefp)(struct kxmlreq *, const struct ical *);

void	 collection_calendar_home_set(struct kxmlreq *);
void	 collection_current_user_principal(struct kxmlreq *);
void	 collection_displayname(struct kxmlreq *);
void	 collection_getctag(struct kxmlreq *);
void	 collection_owner(struct kxmlreq *);
void	 collection_principal_url(struct kxmlreq *);
void	 collection_resourcetype(struct kxmlreq *);
void	 collection_user_address_set(struct kxmlreq *);

void	 resource_calendar_data(struct kxmlreq *, const struct ical *);
void	 resource_getcontenttype(struct kxmlreq *, const struct ical *);
void	 resource_getetag(struct kxmlreq *, const struct ical *);
void	 resource_owner(struct kxmlreq *, const struct ical *);
void	 resource_resourcetype(struct kxmlreq *, const struct ical *);

void	 xml_ical_putc(int, void *);
void	 http_ical_putc(int, void *);

const char *const *xmls;

__END_DECLS

#endif
