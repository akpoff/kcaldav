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
#include "config.h"

#include <sys/stat.h>

#include <assert.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#ifdef __linux__
#include <bsd/string.h>
#endif

#include <kcgi.h>
#include <kcgixml.h>

#include "extern.h"
#include "kcaldav.h"

#ifndef CALDIR
#error "CALDIR token not defined!"
#endif

int verbose = 0;

/*
 * Configure all paths used in our system.
 * We do this here to prevent future mucking around with string copying,
 * concatenation, and path safety.
 * Return zero on failure and non-zero on success.
 */
static int
req2path(struct kreq *r, const char *caldir)
{
	struct state	*st = r->arg;
	size_t	 	 sz;
	int		 machack = 0;
	struct stat	 s;
	char		*cp;

	/* Absolutely don't let it empty paths! */
	if (NULL == r->fullpath || '\0' == r->fullpath[0]) {
		kerrx("zero-length request URI");
		return(0);
	} 

	/* Don't let in paths having relative directory parts. */
	if ('/' != r->fullpath[0]) {
		kerrx("%s: relative path", r->fullpath);
		return(0);
	} else if (strstr(r->fullpath, "../") || 
			strstr(r->fullpath, "/..")) {
		kerrx("%s: insecure path", r->fullpath);
		return(0);
	} 

	/* The filename component can't start with a dot. */
	cp = strrchr(r->fullpath, '/');
	assert(NULL != cp);
	if ('.' == cp[1]) {
		kerrx("%s: dotted filename", r->fullpath);
		return(0);
	}
	
	/* Check our calendar directory for security. */
	if (strstr(caldir, "../") || strstr(caldir, "/..")) {
		kerrx("%s: insecure path", caldir);
		return(0);
	} else if ('\0' == caldir[0]) {
		kerrx("zero-length calendar root");
		return(0);
	}

	/* Create our principal filename. */
	sz = strlcpy(st->prncplfile, caldir, sizeof(st->prncplfile));
	if (sz >= sizeof(st->prncplfile)) {
		kerrx("%s: path too long", st->prncplfile);
		return(0);
	} else if ('/' == st->prncplfile[sz - 1])
		st->prncplfile[sz - 1] = '\0';
	sz = strlcat(st->prncplfile, 
		"/kcaldav.passwd", sizeof(st->prncplfile));
	if (sz > sizeof(st->prncplfile)) {
		kerrx("%s: path too long", st->prncplfile);
		return(0);
	}

	/* Create our file-system mapped pathname. */
	sz = strlcpy(st->path, caldir, sizeof(st->path));
	if ('/' == st->path[sz - 1])
		st->path[sz - 1] = '\0';
	sz = strlcat(st->path, r->fullpath, sizeof(st->path));
	if (sz >= sizeof(st->path)) {
		kerrx("%s: path too long", st->path);
		return(0);
	}

	/* Is the request on a collection? */
	st->isdir = '/' == st->path[sz - 1];

	/*
	 * XXX: Mac OS X iCal hack.
	 * iCal will issue a PROPFIND for a collection and omit the
	 * trailing slash, even when configured otherwise.
	 * Thus, we need to make locally sure that the fullpath appended
	 * to the caldir is a directory or not, then rewrite the request
	 * to reflect this.
	 * This WILL leak information between principals (what is a
	 * directory and what isn't), but for the time being, I don't
	 * know what else to do about it.
	 */
	if ( ! st->isdir && KMETHOD_PROPFIND == r->method) 
		if (-1 != stat(st->path, &s) && S_ISDIR(s.st_mode)) {
			kerrx("MAC OSX HACK");
			st->isdir = 1;
			sz = strlcat(st->path, "/", sizeof(st->path));
			if (sz >= sizeof(st->path)) {
				kerrx("%s: path too long", st->path);
				return(0);
			}
			machack = 1;
		}

	/* Create our full pathname. */
	sz = strlcpy(st->rpath, r->pname, sizeof(st->rpath));
	if (sz && '/' == st->rpath[sz - 1])
		st->rpath[sz - 1] = '\0';
	sz = strlcat(st->rpath, r->fullpath, sizeof(st->rpath));
	if (machack)
		sz = strlcat(st->rpath, "/", sizeof(st->rpath));
	if (sz > sizeof(st->rpath)) {
		kerrx("%s: path too long", st->rpath);
		return(0);
	}

	if ( ! st->isdir) {
		/* Create our file-system mapped temporary pathname. */
		strlcpy(st->temp, st->path, sizeof(st->temp));
		cp = strrchr(st->temp, '/');
		assert(NULL != cp);
		assert('\0' != cp[1]);
		cp[1] = '.';
		cp[2] = '\0';
		strlcat(st->temp, 
			strrchr(st->path, '/') + 1, 
			sizeof(st->temp));
		strlcat(st->temp, ".", sizeof(st->temp));
		sz = strlcat(st->temp, ".XXXXXXXX", sizeof(st->temp));
		if (sz >= sizeof(st->temp)) {
			kerrx("%s: path too long", st->temp);
			return(0);
		}
	}

	/* If we're not a directory, adjust our dir component. */
	strlcpy(st->dir, st->path, sizeof(st->dir));
	if ( ! st->isdir) {
		cp = strrchr(st->dir, '/');
		assert(NULL != cp);
		cp[1] = '\0';
	} 

	/* Create our ctag filename. */
	sz = strlcpy(st->ctagfile, st->dir, sizeof(st->ctagfile));
	if ('/' == st->ctagfile[sz - 1])
		st->ctagfile[sz - 1] = '\0';
	sz = strlcat(st->ctagfile, 
		"/kcaldav.ctag", sizeof(st->ctagfile));
	if (sz >= sizeof(st->ctagfile)) {
		kerrx("%s: path too long", st->ctagfile);
		return(0);
	}

	/* Create our nonce filename. */
	sz = strlcpy(st->noncefile, st->dir, sizeof(st->noncefile));
	if ('/' == st->noncefile[sz - 1])
		st->noncefile[sz - 1] = '\0';
	sz = strlcat(st->noncefile, 
		"/kcaldav.nonce.db", sizeof(st->noncefile));
	if (sz >= sizeof(st->noncefile)) {
		kerrx("%s: path too long", st->noncefile);
		return(0);
	}

	/* Create our configuration filename. */
	sz = strlcpy(st->configfile, st->dir, sizeof(st->configfile));
	if ('/' == st->configfile[sz - 1])
		st->configfile[sz - 1] = '\0';
	sz = strlcat(st->configfile, 
		"/kcaldav.conf", sizeof(st->configfile));
	if (sz >= sizeof(st->configfile)) {
		kerrx("%s: path too long", st->configfile);
		return(0);
	}

	kdbg("path = %s", st->path);
	kdbg("temp = %s", st->temp);
	kdbg("dir = %s", st->dir);
	kdbg("ctagfile = %s", st->ctagfile);
	kdbg("configfile = %s", st->configfile);
	kdbg("prncplfile = %s", st->prncplfile);
	kdbg("rpath = %s", st->rpath);
	kdbg("isdir = %d", st->isdir);

	return(1);
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
		ical = ical_parse(NULL, kp->val, kp->valsz);
		ical_free(ical);
		return(NULL != ical);
	}
	dav = caldav_parse(kp->val, kp->valsz);
	caldav_free(dav);
	return(NULL != dav);
}

int
main(int argc, char *argv[])
{
	struct kreq	 r;
	struct kvalid	 valid = { kvalid, "" };
	struct state	*st;
	int		 rc;
	size_t		 reqsz;
	const char	*caldir, *req;
	char		*np;
	enum nonceerr	 er;

	setlinebuf(stderr);

	while (-1 != getopt(argc, argv, "")) 
		/* Spin. */ ;

	st = NULL;
	caldir = NULL;

	argv += optind;
	if ((argc -= optind) > 0)
		caldir = argv[0];

	if (KCGI_OK != khttp_parse(&r, &valid, 1, NULL, 0, 0))
		return(EXIT_FAILURE);

	kdbg("%s: %s", r.fullpath, 
		KMETHOD__MAX == r.method ? 
		"(unknown)" : kmethods[r.method]);

	/* 
	 * Get the full body of the request, if any.
	 * We'll use this when we're validating the HTTP authorisation.
	 * This will be stored in the fieldmap (or fieldnmap) because we
	 * pass the opaque request body into the kvalid function above.
	 */
	if (NULL != r.fieldmap && NULL != r.fieldmap[0]) {
		req = r.fieldmap[0]->val;
		reqsz = r.fieldmap[0]->valsz;
	} else if (NULL != r.fieldnmap && NULL != r.fieldnmap[0]) {
		req = r.fieldnmap[0]->val;
		reqsz = r.fieldnmap[0]->valsz;
	} else {
		req = "";
		reqsz = 0;
	}

	/* 
	 * Begin by disallowing bogus HTTP methods and processing the
	 * OPTIONS method as well.
	 * Not all agents (e.g., Thunderbird's Lightning) are smart
	 * enough to resend an OPTIONS request with HTTP authorisation,
	 * so let this happen now.
	 */
	if (KMETHOD__MAX == r.method) {
		http_error(&r, KHTTP_405);
		goto out;
	} else if (KMETHOD_OPTIONS == r.method) {
		method_options(&r);
		goto out;
	}

	/* 
	 * Next, require that our HTTP authentication is in place.
	 * If it's not, then we're going to put up a bogus nonce value
	 * so that the client (whomever it is) sends us their login
	 * credentials and we can do more high-level authentication.
	 */
	if (KAUTH_DIGEST != r.rawauth.type) {
		kerrx("%s: HTTP digest required", r.fullpath);
		http_error(&r, KHTTP_401);
		goto out;
	} else if (0 == r.rawauth.authorised) {
		kerrx("%s: bad HTTP authorisation tokens", r.fullpath);
		kerrx("%s: %s", r.fullpath, getenv("HTTP_AUTHORIZATION"));
		http_error(&r, KHTTP_401);
		goto out;
	} 

	/*
	 * Ok, we have enough information to actually begin processing
	 * this client request, so allocate our state.
	 */
	if (NULL == (r.arg = st = calloc(1, sizeof(struct state)))) {
		kerr(NULL);
		http_error(&r, KHTTP_505);
		goto out;
	}

	kdbg("%s: session pre-authorised: %s", 
		r.fullpath, r.rawauth.d.digest.user);

	/*
	 * Resolve paths from the HTTP request.
	 * This sets the many paths we care about--all of which are for
	 * our convenience--and make sure they're secure.
	 */
	if ( ! req2path(&r, NULL == caldir ? CALDIR : caldir)) {
		kerrx("%s: path configuration failure", r.fullpath);
		http_error(&r, KHTTP_403);
		goto out;
	} 

	/* Copy HTTP authorisation. */
	if (r.rawauth.d.digest.alg == KHTTPALG_MD5_SESS)
		st->auth.alg = HTTPALG_MD5_SESS;
	else
		st->auth.alg = HTTPALG_MD5;
	if (r.rawauth.d.digest.qop == KHTTPQOP_NONE)
		st->auth.qop = HTTPQOP_NONE;
	else if (r.rawauth.d.digest.qop == KHTTPQOP_AUTH)
		st->auth.qop = HTTPQOP_AUTH;
	else 
		st->auth.qop = HTTPQOP_AUTH_INT;

	st->auth.user = r.rawauth.d.digest.user;
	st->auth.uri = r.rawauth.d.digest.uri;
	st->auth.realm = r.rawauth.d.digest.realm;
	st->auth.nonce = r.rawauth.d.digest.nonce;
	st->auth.cnonce = r.rawauth.d.digest.cnonce;
	st->auth.response = r.rawauth.d.digest.response;
	st->auth.count = r.rawauth.d.digest.count;
	st->auth.opaque = r.rawauth.d.digest.opaque;

	/*
	 * Next, parse the our passwd file and look up the given HTTP
	 * authorisation name within the database.
	 * This will return -1 on allocation failure or 0 if the password
	 * file doesn't exist or is malformed.
	 * It might set the principal to NULL if not found.
	 */
	rc = prncpl_parse(st->prncplfile, kmethods[r.method], 
		&st->auth, &st->prncpl, req, reqsz);

	if (rc < 0) {
		kerrx("memory failure (principal)");
		http_error(&r, KHTTP_505);
		goto out;
	} else if (0 == rc) {
		kerrx("%s: bad principal file", st->prncplfile);
		http_error(&r, KHTTP_403);
		goto out;
	} else if (NULL == st->prncpl) {
		kerrx("%s: bad principal authorisation: %s", 
			st->prncplfile, st->auth.user);
		http_error(&r, KHTTP_401);
		goto out;
	} else if (strcmp(r.rawauth.d.digest.uri, st->rpath)) {
		kerrx("%s: bad authorisation URI: %s != %s", 
			r.fullpath, r.rawauth.d.digest.uri, st->rpath);
		http_error(&r, KHTTP_401);
		goto out;
	}

	kdbg("%s: session principal authorisation: %s", 
		st->path, st->auth.user);

	/*
	 * Now we see whether our nonce lookup fails.
	 * This is still occuring over a read-only database, as an
	 * adversary could be playing us by submitting replay attacks
	 * (or random nonce values) over and over again in the hopes of
	 * filling up our nonce database.
	 */
	er = nonce_validate(st->noncefile, 
		st->auth.nonce, st->auth.count);

	if (NONCE_ERR == er) {
		kerrx("%s: nonce failure", st->noncefile);
		http_error(&r, KHTTP_505);
		goto out;
	} else if (NONCE_NOTFOUND == er) {
		/*
		 * We don't have the nonce.
		 * This means that the client has either used one of our
		 * bogus initial nonces or is using one from a much
		 * earlier session.
		 * Tell them to retry with a new nonce.
		 */
		if ( ! nonce_new(st->noncefile, &np)) {
			kerrx("%s: nonce failure", st->noncefile);
			http_error(&r, KHTTP_505);
			goto out;
		}
		kdbg("%s: sending new nonce: %s", st->path, np);
		khttp_head(&r, kresps[KRESP_STATUS], 
			"%s", khttps[KHTTP_401]);
		khttp_head(&r, kresps[KRESP_WWW_AUTHENTICATE],
			"Digest realm=\"%s\" algorithm=MD5-sess "
			"qop=\"auth,auth-int\" nonce=\"%s\" "
			"stale=true", KREALM, np);
		khttp_body(&r);
		goto out;
	} else if (NONCE_REPLAY == er) {
		kerrx("%s: REPLAY ATTACK: %s\n",
			st->noncefile, st->auth.user);
		http_error(&r, KHTTP_403);
		goto out;
	} 

	kdbg("%s: session nonce pre-authorised: %s", 
		st->path, st->auth.user);

	/*
	 * Now we actually update our nonce file.
	 * We only get here if the nonce value exists and is fresh.
	 */
	er = nonce_update(st->noncefile,
		st->auth.nonce, st->auth.count);

	if (NONCE_ERR == er) {
		kerrx("%s: nonce failure", st->noncefile);
		http_error(&r, KHTTP_505);
		goto out;
	} else if (NONCE_NOTFOUND == er) {
		kdbg("%s: nonce update not found?", st->noncefile);
		if ( ! nonce_new(st->noncefile, &np)) {
			kerrx("%s: nonce failure", st->noncefile);
			http_error(&r, KHTTP_505);
			goto out;
		}
		kdbg("%s: sending new nonce: %s", st->path, np);
		khttp_head(&r, kresps[KRESP_STATUS], 
			"%s", khttps[KHTTP_401]);
		khttp_head(&r, kresps[KRESP_WWW_AUTHENTICATE],
			"Digest realm=\"%s\" algorithm=MD5-sess "
			"qop=\"auth,auth-int\" nonce=\"%s\" "
			"stale=true", KREALM, np);
		khttp_body(&r);
		goto out;
	} else if (NONCE_REPLAY == er) {
		kerrx("%s: REPLAY ATTACK: %s\n",
			st->noncefile, st->auth.user);
		http_error(&r, KHTTP_403);
		goto out;
	} 

	kdbg("%s: session nonce authorised: %s", 
		st->path, st->auth.user);

	/* 
	 * We require a configuration file in the directory where we've
	 * been requested to introspect.
	 * It's ok if "path" is a directory.
	 */
	rc = config_parse(st->configfile, &st->cfg, st->prncpl);

	if (rc < 0) {
		kerrx("memory failure (config)");
		http_error(&r, KHTTP_505);
		goto out;
	} else if (0 == rc) {
		kerrx("%s: bad config", st->configfile);
		http_error(&r, KHTTP_403);
		goto out;
	} else if (PERMS_NONE == st->cfg->perms) {
		kerrx("%s: principal without privilege: %s", 
			st->path, st->prncpl->name);
		http_error(&r, KHTTP_403);
		goto out;
	}

	kdbg("%s: configuration parsed", st->path);

	/* 
	 * We're ready to go!
	 * (We still may fail privileges for individual resources, but
	 * beyond that, the only failure is on the request URI.)
	 */
	switch (r.method) {
	case (KMETHOD_PUT):
		method_put(&r);
		break;
	case (KMETHOD_PROPFIND):
		method_propfind(&r);
		break;
	case (KMETHOD_GET):
		method_get(&r);
		break;
	case (KMETHOD_REPORT):
		method_report(&r);
		break;
	case (KMETHOD_DELETE):
		method_delete(&r);
		break;
	default:
		kerrx("%s: ignoring method %s",
			st->path, kmethods[r.method]);
		http_error(&r, KHTTP_405);
		break;
	}

out:
	khttp_free(&r);
	if (NULL != st) {
		config_free(st->cfg);
		prncpl_free(st->prncpl);
		free(st);
	}
	return(EXIT_SUCCESS);
}
