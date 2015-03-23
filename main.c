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
#include <sys/types.h>
#include <sys/stat.h>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <paths.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <kcgi.h>
#include <kcgixml.h>

#include "extern.h"

#ifndef CALDIR
#error "CALDIR token not defined!"
#endif

static	const char *const kmethods[KMETHOD__MAX + 1] = {
	"acl", /* KMETHOD_ACL */
	"connect", /* KMETHOD_CONNECT */
	"copy", /* KMETHOD_COPY */
	"delete", /* KMETHOD_DELETE */
	"get", /* KMETHOD_GET */
	"head", /* KMETHOD_HEAD */
	"lock", /* KMETHOD_LOCK */
	"mkcalendar", /* KMETHOD_MKCALENDAR */
	"mkcol", /* KMETHOD_MKCOL */
	"move", /* KMETHOD_MOVE */
	"options", /* KMETHOD_OPTIONS */
	"post", /* KMETHOD_POST */
	"propfind", /* KMETHOD_PROPFIND */
	"proppatch", /* KMETHOD_PROPPATCH */
	"put", /* KMETHOD_PUT */
	"report", /* KMETHOD_REPORT */
	"trace", /* KMETHOD_TRACE */
	"unlock", /* KMETHOD_UNLOCK */
	"unknown", /* KMETHOD__MAX */
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

static	const char *const xmls[XML__MAX] = {
	"C:calendar", /* XML_CALDAV_CALENDAR */
	"D:collection", /* XML_DAV_COLLECTION */
	"D:href", /* XML_DAV_HREF */
	"D:multistatus", /* XML_DAV_MULTISTATUS */
	"D:prop", /* XML_DAV_PROP */
        "D:propstat", /* XML_DAV_PROPSTAT */
	"D:response", /* XML_DAV_RESPONSE */
	"D:status", /* XML_DAV_STATUS */
	"D:unauthenticated", /* XML_DAV_UNAUTHENTICATED */
};

static int
pathnorm(const char *p, char *path)
{
	int	 sz;

	if (NULL == p || '\0' == p[0])
		return(0);
	if (strstr(p, "../") || strstr(p, "/.."))
		return(0);
	sz = snprintf(path, PATH_MAX, "%s%s", CALDIR, p);
	return(sz < PATH_MAX);
}

/* 
 * Validator for iCalendar OR CalDav object.
 * This checks the initial string of the object: if it equals the
 * prologue to an iCalendar, we attempt an iCalendar parse.
 * Otherwise, we try a CalDav parse.
 */
static int
kvalid(struct kpair *kp)
{
	size_t	 	 sz;
	const char	*tok = "BEGIN:VCALENDAR";
	struct ical	*ical;
	struct caldav	*dav;

	if ( ! kvalid_stringne(kp))
		return(0);
	sz = strlen(tok);
	if (0 == strncmp(kp->val, tok, sz)) {
		ical = ical_parse(kp->val);
		ical_free(ical);
		return(NULL != ical);
	}
	dav = caldav_parse(kp->val, kp->valsz);
	caldav_free(dav);
	return(NULL != dav);
}

/*
 * Put a character into a request stream.
 * We use this instead of just fputc because the stream might be
 * compressed, so we need kcgi(3)'s function.
 */
static void
ical_putc(int c, void *arg)
{
	struct kreq	*r = arg;

	khttp_putc(r, c);
}

/*
 * If found, convert the (already-validated) iCalendar.
 */
static struct ical *
req2ical(struct kreq *r)
{
	struct ical	*p;

	if (NULL == r->fieldmap[0]) {
		khttp_head(r, kresps[KRESP_STATUS], 
			"%s", khttps[KHTTP_400]);
		khttp_body(r);
		return(NULL);
	}
	
	/* We already know that this works! */
	p = ical_parse(r->fieldmap[0]->val);
	assert(NULL != p);
	return(p);
}

/*
 * This converts the request into a CalDav object.
 * We know that the request is a well-formed CalDav object because it
 * appears in the fieldmap and we parsed it during HTTP unpacking.
 * So we really only check its media type.
 */
static struct caldav *
req2caldav(struct kreq *r, enum type type)
{
	struct caldav	*p;

	if (NULL == r->fieldmap[0]) {
		khttp_head(r, kresps[KRESP_STATUS], 
			"%s", khttps[KHTTP_400]);
		khttp_body(r);
		return(NULL);
	} 

	p = caldav_parse
		(r->fieldmap[0]->val, 
		 r->fieldmap[0]->valsz);
	assert(NULL != p);

	/* Check our media type. */
	if (type != p->type) {
		khttp_head(r, kresps[KRESP_STATUS], 
			"%s", khttps[KHTTP_415]);
		khttp_body(r);
		caldav_free(p);
		return(NULL);
	}

	return(p);
}

/*
 * Satisfy RFC 4791, 5.3.2, PUT.
 * This has fairly complicated semantics.
 */
static void
put(struct kreq *r)
{
	struct ical	*p, *cur;
	int		 fd;
	size_t		 sz;
	const char	*d;

	if (NULL == (p = req2ical(r)))
		return;

	if (NULL != r->reqmap[KREQU_IF] &&
		(sz = strlen(r->reqmap[KREQU_IF]->val)) == 36 &&
		'(' == r->reqmap[KREQU_IF]->val[0] &&
		'[' == r->reqmap[KREQU_IF]->val[1] &&
		']' == r->reqmap[KREQU_IF]->val[sz - 2] &&
		')' == r->reqmap[KREQU_IF]->val[sz - 1]) {
		/*
		 * If we have an If header, then we see if the existing
		 * object matches the requested Etag (MD5).  If it does,
		 * then we replace it with the PUT.  If it doesn't, then
		 * we don't do the replacement.
		 */
		cur = ical_parsefile_open(r->arg, &fd);
		d = r->reqmap[KREQU_IF]->val + 2;
		if (NULL == cur) {
			khttp_head(r, kresps[KRESP_STATUS], 
				"%s", khttps[KHTTP_404]);
		} else if (strncmp(cur->digest, d, 32)) {
			khttp_head(r, kresps[KRESP_STATUS], 
				"%s", khttps[KHTTP_412]);
		} else if (-1 == lseek(fd, 0, SEEK_SET) ||
				-1 == ftruncate(fd, 0)) {
			perror(r->arg);
			khttp_head(r, kresps[KRESP_STATUS], 
				"%s", khttps[KHTTP_505]);
		} else {
			ical_printfile(fd, p);
			khttp_head(r, kresps[KRESP_STATUS], 
				"%s", khttps[KHTTP_201]);
			khttp_head(r, kresps[KRESP_ETAG], 
				"%s", p->digest);
		}
		ical_parsefile_close(r->arg, fd);
		ical_free(cur);
	} else if (ical_putfile(r->arg, p)) {
		khttp_head(r, kresps[KRESP_STATUS], 
			"%s", khttps[KHTTP_201]);
		khttp_head(r, kresps[KRESP_ETAG], 
			"%s", p->digest);
	} else if (NULL == (cur = ical_parsefile(r->arg))) {
		khttp_head(r, kresps[KRESP_STATUS], 
			"%s", khttps[KHTTP_505]);
	} else {
		khttp_head(r, kresps[KRESP_STATUS], 
			"%s", khttps[KHTTP_412]);
		khttp_head(r, kresps[KRESP_ETAG], 
			"%s", cur->digest);
		ical_free(cur);
	}
	khttp_body(r);
	ical_free(p);
}

/*
 * RFC 4791 doesn't specify any special behaviour for GET, and RFC 2518
 * just says it follows HTTP, RFC 2068.
 * So use those semantics.
 */
static void
get(struct kreq *r)
{
	struct ical	*p;
	const char	*cp;

	if (NULL == (p = ical_parsefile(r->arg))) {
		khttp_head(r, kresps[KRESP_STATUS], 
			"%s", khttps[KHTTP_404]);
		khttp_body(r);
		return;
	}

	/* 
	 * If we request with the If-None-Match header (see RFC 2068,
	 * 14.26), then see if the ETag (MD5 hash) is consistent.
	 * If it is, then indicate that the remote cache is ok.
	 * If not, resend the data.
	 */
	if (NULL != r->reqmap[KREQU_IF_NONE_MATCH]) {
		cp = r->reqmap[KREQU_IF_NONE_MATCH]->val;
		if (0 == strcmp(p->digest, cp)) {
			khttp_head(r, kresps[KRESP_STATUS], 
				"%s", khttps[KHTTP_304]);
			khttp_head(r, kresps[KRESP_ETAG], 
				"%s", p->digest);
			khttp_body(r);
			ical_free(p);
			return;
		} 
	} 

	khttp_head(r, kresps[KRESP_STATUS], 
		"%s", khttps[KHTTP_200]);
	khttp_head(r, kresps[KRESP_CONTENT_TYPE], 
		"%s", kmimetypes[KMIME_TEXT_CALENDAR]);
	khttp_head(r, kresps[KRESP_ETAG], "%s", p->digest);
	khttp_body(r);
	ical_print(p, ical_putc, r);
	ical_free(p);
}

/*
 * Defines the PROPFIND method for a collection (directory).
 */
static void
propfind_collection(const struct caldav *dav, struct kreq *r)
{
	size_t	 	 i, nf;
	struct kxmlreq	 xml;
	struct config	*cfg;
	const char	*tag;
	int 		 accepted[PROP__MAX];

	memset(accepted, 0, sizeof(accepted));
	accepted[PROP_CALENDAR_HOME_SET] = 1;
	accepted[PROP_CALENDAR_USER_ADDRESS_SET] = 1;
	accepted[PROP_CURRENT_USER_PRINCIPAL] = 1;
	accepted[PROP_DISPLAYNAME] = 1;
	accepted[PROP_EMAIL_ADDRESS_SET] = 1;
	accepted[PROP_PRINCIPAL_URL] = 1;
	accepted[PROP_RESOURCETYPE] = 1;

	/*
	 * If we can't find the configuration file within this current
	 * directory, then exit with an HTTP 404.
	 */
	if (NULL == (cfg = config_parse(r->fullpath, r->arg))) {
		khttp_head(r, kresps[KRESP_STATUS], 
			"%s", khttps[KHTTP_403]);
		khttp_body(r);
		return;
	}

	khttp_head(r, kresps[KRESP_STATUS], 
		"%s", khttps[KHTTP_207]);
	khttp_head(r, kresps[KRESP_CONTENT_TYPE], 
		"%s", kmimetypes[KMIME_TEXT_XML]);

	khttp_body(r);
	kxml_open(&xml, r, xmls, XML__MAX);
	kxml_pushattrs(&xml, XML_DAV_MULTISTATUS, 
		"xmlns:B", "http://calendarserver.org/ns/",
		"xmlns:C", "urn:ietf:params:xml:ns:caldav",
		"xmlns:D", "DAV:", NULL);
	kxml_push(&xml, XML_DAV_RESPONSE);
	kxml_push(&xml, XML_DAV_HREF);
	kxml_text(&xml, "http://");
	kxml_text(&xml, r->host);
	kxml_text(&xml, r->pname);
	kxml_text(&xml, r->fullpath);
	kxml_pop(&xml);
	kxml_push(&xml, XML_DAV_PROPSTAT);
	kxml_push(&xml, XML_DAV_PROP);
	for (nf = i = 0; i < dav->propsz; i++) {
		if ( ! accepted[dav->props[i].key]) {
			nf++;
			continue;
		}
		tag = calelems[calpropelems[dav->props[i].key]];
		khttp_puts(r, "<X:");
		khttp_puts(r, tag);
		khttp_puts(r, " xmlns:X=\"");
		khttp_puts(r, dav->props[i].xmlns);
		khttp_puts(r, "\">");
		switch (dav->props[i].key) {
		case (PROP_CALENDAR_HOME_SET):
			kxml_push(&xml, XML_DAV_HREF);
			kxml_text(&xml, "http://");
			kxml_text(&xml, r->host);
			kxml_text(&xml, r->pname);
			kxml_text(&xml, cfg->calendarhome);
			kxml_pop(&xml);
			break;
		case (PROP_CALENDAR_USER_ADDRESS_SET):
			kxml_text(&xml, cfg->calendaruseraddress);
			break;
		case (PROP_CURRENT_USER_PRINCIPAL):
			kxml_pushnull(&xml, XML_DAV_UNAUTHENTICATED);
			break;
		case (PROP_DISPLAYNAME):
			kxml_text(&xml, cfg->displayname);
			break;
		case (PROP_EMAIL_ADDRESS_SET):
			kxml_text(&xml, cfg->emailaddress);
			break;
		case (PROP_PRINCIPAL_URL):
			kxml_push(&xml, XML_DAV_HREF);
			kxml_text(&xml, "http://");
			kxml_text(&xml, r->host);
			kxml_text(&xml, r->pname);
			kxml_text(&xml, r->fullpath);
			kxml_pop(&xml);
			break;
		case (PROP_RESOURCETYPE):
			kxml_pushnull(&xml, XML_DAV_COLLECTION);
			kxml_pushnull(&xml, XML_CALDAV_CALENDAR);
			break;
		default:
			abort();
			break;
		} 
		khttp_puts(r, "</X:");
		khttp_puts(r, tag);
		khttp_putc(r, '>');
	}
	kxml_pop(&xml);
	kxml_push(&xml, XML_DAV_STATUS);
	kxml_text(&xml, "HTTP/1.1 ");
	kxml_text(&xml, khttps[KHTTP_200]);
	kxml_pop(&xml);
	kxml_pop(&xml);

	/*
	 * If we had any properties for which we're not going to report,
	 * then indicate that now with a 404.
	 */
	if (nf > 0) {
		kxml_push(&xml, XML_DAV_PROPSTAT);
		kxml_push(&xml, XML_DAV_PROP);
		for (i = 0; i < dav->propsz; i++) {
			if (accepted[dav->props[i].key])
				continue;
			khttp_puts(r, "<X:");
			khttp_puts(r, dav->props[i].name);
			khttp_puts(r, " xmlns:X=\"");
			khttp_puts(r, dav->props[i].xmlns);
			khttp_puts(r, "\" />");
		}
		kxml_pop(&xml);
		kxml_push(&xml, XML_DAV_STATUS);
		kxml_text(&xml, "HTTP/1.1 ");
		kxml_text(&xml, khttps[KHTTP_404]);
	}

	kxml_popall(&xml);
	kxml_close(&xml);
	config_free(cfg);
}

/*
 * A PROPFIND request for a specified calendar resource (file).
 */
static void
propfind_resource(const struct caldav *dav, struct kreq *r)
{
	struct ical	*ical;
	size_t		 i, nf;
	const char	*tag;
	struct kxmlreq	 xml;

	/* We can only request iCal object, so parse now. */
	if (NULL == (ical = ical_parsefile(r->arg))) {
		khttp_head(r, kresps[KRESP_STATUS], 
			"%s", khttps[KHTTP_403]);
		khttp_body(r);
		return;
	} 

	khttp_head(r, kresps[KRESP_STATUS], 
		"%s", khttps[KHTTP_207]);
	khttp_head(r, kresps[KRESP_CONTENT_TYPE], 
		"%s", kmimetypes[KMIME_TEXT_XML]);
	khttp_body(r);
	kxml_open(&xml, r, xmls, XML__MAX);
	kxml_pushattrs(&xml, XML_DAV_MULTISTATUS, 
		"xmlns:B", "http://calendarserver.org/ns/",
		"xmlns:C", "urn:ietf:params:xml:ns:caldav",
		"xmlns:D", "DAV:", NULL);
	kxml_push(&xml, XML_DAV_RESPONSE);
	kxml_push(&xml, XML_DAV_HREF);
	kxml_text(&xml, "http://");
	kxml_text(&xml, r->host);
	kxml_text(&xml, r->pname);
	kxml_text(&xml, r->fullpath);
	kxml_pop(&xml);
	kxml_push(&xml, XML_DAV_PROPSTAT);
	kxml_push(&xml, XML_DAV_PROP);
	for (nf = i = 0; i < dav->propsz; i++) {
		if (PROP_GETETAG != dav->props[i].key) {
			nf++;
			continue;
		}
		tag = calelems[calpropelems[dav->props[i].key]];
		khttp_puts(r, "<X:");
		khttp_puts(r, tag);
		khttp_puts(r, " xmlns:X=\"");
		khttp_puts(r, dav->props[i].xmlns);
		khttp_puts(r, "\">");
		khttp_puts(r, ical->digest);
		khttp_puts(r, "</X:");
		khttp_puts(r, tag);
		khttp_putc(r, '>');
	}
	kxml_pop(&xml);
	kxml_push(&xml, XML_DAV_STATUS);
	kxml_text(&xml, "HTTP/1.1 ");
	kxml_text(&xml, khttps[KHTTP_200]);
	kxml_pop(&xml);
	kxml_pop(&xml);

	/*
	 * If we had any properties for which we're not going to report,
	 * then indicate that now with a 404.
	 */
	if (nf > 0) {
		kxml_push(&xml, XML_DAV_PROPSTAT);
		kxml_push(&xml, XML_DAV_PROP);
		for (i = 0; i < dav->propsz; i++) {
			/* Note which properties we supported. */
			if (PROP_GETETAG == dav->props[i].key) 
				continue;
			
			/* Otherwise... */
			khttp_puts(r, "<X:");
			khttp_puts(r, dav->props[i].name);
			khttp_puts(r, " xmlns:X=\"");
			khttp_puts(r, dav->props[i].xmlns);
			khttp_puts(r, "\" />");
		}
		kxml_pop(&xml);
		kxml_push(&xml, XML_DAV_STATUS);
		kxml_text(&xml, "HTTP/1.1 ");
		kxml_text(&xml, khttps[KHTTP_404]);
	}

	kxml_popall(&xml);
	kxml_close(&xml);
	ical_free(ical);
}

/*
 * PROPFIND is used to define properties for calendar collections (i.e.,
 * directories consisting of calendar resources) or resources
 * themselves.
 * Switch on that behaviour here.
 */
static void
propfind(struct kreq *r)
{
	struct caldav	*dav;
	struct stat	 st;

	/* Resource doesn't exist or request doesn't parse. */
	if (-1 == stat(r->arg, &st)) {
		khttp_head(r, kresps[KRESP_STATUS], 
			"%s", khttps[KHTTP_403]);
		khttp_body(r);
		return;
	} else if (NULL == (dav = req2caldav(r, TYPE_PROPFIND)))
		return;

	/* Pass request to collection/resource. */
	if (S_IFDIR & st.st_mode)
		propfind_collection(dav, r);
	else
		propfind_resource(dav, r);

	caldav_free(dav);
}

static void
options(struct kreq *r)
{
	struct config	*cfg;

	if (NULL != (cfg = config_parse(r->fullpath, r->arg))) {
		khttp_head(r, kresps[KRESP_STATUS], 
			"%s", khttps[KHTTP_200]);
		khttp_head(r, kresps[KRESP_ALLOW], "%s", 
			"OPTIONS, GET, PUT, PROPFIND");
		khttp_head(r, "DAV", "1, 2, "
			"access-control, calendar-access");
	} else
		khttp_head(r, kresps[KRESP_STATUS], 
			"%s", khttps[KHTTP_404]);

	khttp_body(r);
	config_free(cfg);
}

int
main(void)
{
	struct kreq	 r;
	struct kvalid	 valid = { kvalid, "" };
	char		 path[PATH_MAX];

	if (KCGI_OK != khttp_parse(&r, &valid, 1, NULL, 0, 0))
		return(EXIT_FAILURE);

	/*
	 * Begin by resolving a path from the HTTP request.
	 * We don't allow backtracking paths, and we reconstitute all of
	 * our paths to be in the CALDIR preprocessor variable.
	 */
	if ( ! pathnorm(r.fullpath, path)) {
		fprintf(stderr, "%s: insecure path\n", r.fullpath);
		khttp_head(&r, kresps[KRESP_STATUS], 
			"%s", khttps[KHTTP_404]);
		khttp_body(&r);
		khttp_free(&r);
		return(EXIT_FAILURE);
	} else
		r.arg = path;

	switch (r.method) {
	case (KMETHOD_OPTIONS):
		options(&r);
		break;
	case (KMETHOD_PUT):
		put(&r);
		break;
	case (KMETHOD_PROPFIND):
		propfind(&r);
		break;
	case (KMETHOD_GET):
		get(&r);
		break;
	default:
		khttp_head(&r, kresps[KRESP_STATUS], 
			"%s", khttps[KHTTP_405]);
		khttp_body(&r);
		break;
	}

	khttp_free(&r);
	return(EXIT_SUCCESS);
}
