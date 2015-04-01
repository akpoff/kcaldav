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
#include <sys/stat.h>
#include <sys/mman.h>

#include <assert.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "extern.h"
#include "md5.h"

static void
icalnode_free(struct icalnode *p, int first)
{
	struct icalnode	*np;

	if (NULL == p)
		return;
	np = p->next;
	icalnode_free(p->first, 0);
	free(p->name);
	free(p->val);
	free(p);
	if (0 == first)
		icalnode_free(np, 0);
}

static struct icalnode *
icalnode_clone(const struct icalnode *p, int first)
{
	struct icalnode	*np;

	assert(NULL != p);

	if (NULL == (np = calloc(1, sizeof(struct icalnode)))) {
		perror(NULL);
		return(np);
	}

	np->name = strdup(p->name);
	np->val = strdup(p->val);
	if (NULL != p->first) {
		np->first = icalnode_clone(p->first, 0);
		if (NULL == np->first) {
			icalnode_free(np, 0);
			return(NULL);
		}
		np->first->parent = np;
	}
	if (0 == first && NULL != p->next) {
		np->next = icalnode_clone(p->next, 0);
		if (NULL == np->next) {
			icalnode_free(np, 0);
			return(NULL);
		}
		np->next->parent = np->parent;
	}
	return(np);
}

void
ical_free(struct ical *p)
{

	if (NULL == p)
		return;
	
	icalnode_free(p->first, 0);
	free(p);
}

static int
ical_line(const char **cp, struct buf *buf)
{
	const char	*end, *start;

	buf->sz = 0;
	start = *cp;
	for (;;) {
		if (NULL == (end = strstr(start, "\r\n")))
			return(0);
		bufappend(buf, start, end - start);
		if (' ' != end[2] && '\t' != end[2]) 
			break;
		start = end + 2;
		while (' ' == *start || '\t' == *start)
			start++;
	}

	*cp = end + 2;
	return(1);
}

struct ical *
ical_parsefile(const char *file)
{


	return(ical_parsefile_open(file, NULL));
}

int
ical_parsefile_close(const char *file, int fd)
{

	if (-1 == fd)
		return(1);

	if (-1 == flock(fd, LOCK_UN)) {
		perror(file);
		close(fd);
		return(0);
	} 
	close(fd);
	return(1);
}

struct ical *
ical_parsefile_open(const char *file, int *keep)
{
	int	 	 fd;
	unsigned int	 flags;
	char		*buf;
	struct stat	 st;
	ssize_t		 ssz;
	struct ical	*p;

	if (NULL != keep) {
		*keep = -1;
		flags = O_RDWR | O_EXLOCK;
	} else
		flags = O_RDONLY | O_SHLOCK;

	if (-1 == (fd = open(file, flags, 0600))) {
		perror(file);
		return(NULL);
	} else if (-1 == fstat(fd, &st)) {
		perror(file);
		flock(fd, LOCK_UN);
		close(fd);
		return(NULL);
	} else if (NULL == (buf = malloc(st.st_size + 1))) {
		perror(NULL);
		flock(fd, LOCK_UN);
		close(fd);
		return(NULL);
	} else if ((ssz = read(fd, buf, st.st_size)) < 0) {
		perror(file);
		flock(fd, LOCK_UN);
		close(fd);
		free(buf);
		return(NULL);
	} else if (ssz != st.st_size) {
		fprintf(stderr, "%s: invalid read size\n", file);
		flock(fd, LOCK_UN);
		close(fd);
		free(buf);
		return(NULL);
	}
	buf[st.st_size] = '\0';
	if (-1 == flock(fd, LOCK_UN)) {
		perror(file);
		close(fd);
		free(buf);
		return(NULL);
	} else if (NULL == keep)
		close(fd);

	p = ical_parse(buf);
	free(buf);
	if (NULL != keep)
		*keep = fd;
	return(p);
}

struct ical *
ical_parse(const char *cp)
{
	struct ical	*p;
	struct icalnode	*cur, *np;
	char		*name, *val;
	struct buf	 buf;
	MD5_CTX		 ctx;
	size_t		 i;
	unsigned char	 digest[16];

	memset(&buf, 0, sizeof(struct buf));

	if (NULL == (p = calloc(1, sizeof(struct ical)))) {
		perror(NULL);
		return(NULL);
	}

	MD5Init(&ctx);
	MD5Update(&ctx, cp, strlen(cp));
	MD5Final(digest, &ctx);
	for (i = 0; i < sizeof(digest); i++) 
	         snprintf(&p->digest[i * 2], 3, "%02x", digest[i]);

	cur = NULL;
	while ('\0' != *cp) {
		if (0 == ical_line(&cp, &buf)) {
			fprintf(stderr, "unterminated line\n");
			break;
		}

		name = buf.buf;
		assert(NULL != name);

		if (NULL != (val = strchr(name, ':')))
			*val++ = '\0';

		if (NULL == val) {
			fprintf(stderr, "no value\n");
			break;
		}

		np = calloc(1, sizeof(struct icalnode));

		if (NULL == np) {
			perror(NULL);
			break;
		} else if (NULL == (np->name = strdup(name))) {
			perror(NULL);
			icalnode_free(np, 0);
			break;
		} else if (val && NULL == (np->val = strdup(val))) {
			perror(NULL);
			icalnode_free(np, 0);
			break;
		}

		if (NULL == p->first) {
			p->first = cur = np;
			continue;
		} else if (NULL == cur) {
			fprintf(stderr, "bad nesting\n");
			break;
		}

		if (0 == strcmp("BEGIN", cur->name)) {
			if (0 == strcmp("END", np->name)) {
				np->parent = cur->parent;
				cur->next = np;
				cur = np;
				continue;
			}
			cur->first = np;
			np->parent = cur;
			cur = np;
		} 
		
		if (0 == strcmp("END", np->name))
			cur = cur->parent;

		np->parent = cur->parent;
		cur->next = np;
		cur = np;
	}

	free(buf.buf);

	if (NULL == p->first) {
		fprintf(stderr, "empty icalendar\n");
		ical_free(p);
		return(NULL);
	} else if (strcmp(p->first->name, "BEGIN") ||
		 strcmp(p->first->val, "VCALENDAR")) {
		fprintf(stderr, "bad icalendar root\n");
		ical_free(p);
		return(NULL);
	} else if (NULL != cur->parent) {
		fprintf(stderr, "bad nesting\n");
		ical_free(p);
		return(NULL);
	} else if ('\0' != *cp) {
		fprintf(stderr, "bad parse\n");
		ical_free(p);
		return(NULL);
	}

	return(p);
}

static int
icalnode_putc(int c, void *arg)
{
	int	 fd = *(int *)arg;

	return(write(fd, &c, 1));
}

static int
icalnode_putchar(char c, size_t *col, ical_putchar fp, void *arg)
{

	if (74 == *col) {
		if ((*fp)('\r', arg) < 0)
			return(-1);
		if ((*fp)('\n', arg) < 0)
			return(-1);
		if ((*fp)(' ', arg) < 0)
			return(-1);
		*col = 1;
	}
	if ((*fp)(c, arg) < 0)
		return(-1);
	(*col)++;
	return(1);
}

static int
icalnode_print(const struct icalnode *p, ical_putchar fp, void *arg)
{
	const char	*cp;
	size_t		 col;

	if (NULL == p) 
		return(1);

	col = 0;
	for (cp = p->name; '\0' != *cp; cp++) 
		if (icalnode_putchar(*cp, &col, fp, arg) < 0)
			return(-1);
	if (icalnode_putchar(':', &col, fp, arg) < 0)
		return(-1);
	for (cp = p->val; '\0' != *cp; cp++) 
		if (icalnode_putchar(*cp, &col, fp, arg) < 0)
			return(-1);
	if ((*fp)('\r', arg) < 0)
		return(-1);
	if ((*fp)('\n', arg) < 0)
		return(-1);
	if (icalnode_print(p->first, fp, arg) < 0)
		return(-1);
	if (icalnode_print(p->next, fp, arg) < 0)
		return(-1);
	return(1);
}

int
ical_print(const struct ical *p, ical_putchar fp, void *arg)
{

	return(icalnode_print(p->first, fp, arg));
}

int
ical_printfile(int fd, const struct ical *p)
{

	return(icalnode_print(p->first, icalnode_putc, &fd));
}

int
ical_merge(struct ical *p, const struct ical *newp)
{
	const struct icalnode	*newpp, *nnp, *xp;
	const char		*uid;
	struct icalnode		*np, *begin, *end;

	assert(NULL != newp->first);
	newpp = newp->first;
	assert(NULL != p->first);

	/* Scan through all of the NEW requests. */
	for (newpp = newpp->first; NULL != newpp; newpp = newpp->next) {
		/* 
		 * For the time being, we only handle new VEVENT's. 
		 * So look for those.
		 */
		if (strcmp(newpp->name, "BEGIN"))
			continue;
		else if (strcmp(newpp->val, "VEVENT"))
			continue;

		/* Make sure they have a UID. */
		for (nnp = newpp->first; NULL != nnp; nnp = nnp->next)
			if (0 == strcmp("UID", nnp->name))
				break;

		/* This shouldn't happen... */
		if (NULL == nnp)
			continue;
		uid = nnp->val;

		/* Now clone the event (and it's END) to merge. */
		if (NULL == (begin = icalnode_clone(newpp, 1)))
			return(0);
		assert(newpp->next);
		assert(0 == strcmp(newpp->next->name, "END"));
		if (NULL == (end = icalnode_clone(newpp->next, 1))) {
			icalnode_free(begin, 0);
			return(0);
		}
		begin->next = end;

		/* Look for the VEVENT in our existing database. */
		for (nnp = p->first->first; NULL != nnp; nnp = nnp->next) {
			/* Again, only look for VEVENTs. */
			if (strcmp(nnp->name, "BEGIN"))
				continue;
			else if (strcmp(nnp->val, "VEVENT"))
				continue;
			/* Match UID... */
			for (xp = nnp->first; NULL != xp; xp = xp->next)
				if (0 == strcmp("UID", xp->name))
					break;
			if (NULL != xp && 0 == strcmp(uid, xp->val))
				break;
		}
		
		/* Check if we've been handed a duplicate VEVENT. */
		if (NULL != nnp) {
			icalnode_free(begin, 0);
			continue;
		}

		/* Find the place to merge the new VEVENT. */
		for (np = p->first->first; NULL != np; np = np->next) 
			if (NULL == np->next)
				break;

		/* Insert as first or subsequent entry. */
		if (NULL == np) {
			begin->parent = p->first;
			end->parent = p->first;
			p->first->first = begin;
		} else {
			begin->parent = np->parent;
			end->parent = np->parent;
			end->next = np->next;
			np->next = begin;
		}
	}
	return(1);
}

int
ical_putfile(const char *file, const struct ical *newp)
{
	int	 fd, flags;

	flags = O_RDWR | O_EXLOCK | O_CREAT | O_EXCL;

	/* Open our database with an EXLOCK. */
	if (-1 == (fd = open(file, flags, 0600))) {
		perror(file);
		return(0);
	}

	ical_printfile(fd, newp);

	if (-1 == flock(fd, LOCK_UN)) 
		perror(file);

	close(fd);
	return(1);
}

#if 0
int
ical_mergefile(const char *file, const struct ical *newp)
{
	int		 fd, flags, rc;
	struct stat	 st;
	size_t		 sz;
	char		*buf;
	struct ical	*p;
	FILE		*f;
	ssize_t		 ssz;

	rc = 0;
	p = NULL;
	flags = O_RDWR | O_EXLOCK | O_CREAT;
	buf = NULL;
	f = NULL;

	/* Open our database with an EXLOCK. */
	if (-1 == (fd = open(file, flags, 0666))) {
		perror(file);
		return(0);
	} else if (-1 == fstat(fd, &st)) {
		perror(file);
		goto out;
	} else if (0 == st.st_size) {
		/* 
		 * If the file is new (i.e., empty), then just duplicate
		 * the entry and push it to the disc.
		 */
		if (NULL == (f = fdopen(fd, "w"))) {
			perror(file);
			goto out;
		}
		ical_printfile(f, newp);
		rc = 1;
		goto out;
	} else
		fprintf(stderr, "%s: merge write\n", file);

	/*
	 * Read into a nil-terminated buffer. 
	 * Once we're finished reading, seek back to the beginning of
	 * the file stream for our re-write.
	 */
	sz = (size_t)st.st_size;
	if (NULL == (buf = malloc(sz + 1))) {
		perror(NULL);
		goto out;
	} else if ((ssz = read(fd, buf, sz)) < 0) {
		perror(file);
		goto out;
	} else if ((size_t)ssz != sz) {
		fprintf(stderr, "%s: short read\n", file);
		goto out;
	} else if (-1 == lseek(fd, 0, SEEK_SET)) {
		perror(file);
		goto out;
	}
	buf[sz] = '\0';

	/* If we can parse our database, merge it. */
	p = ical_parse(buf);
	if (NULL == p || ! ical_merge(fd, p, newp))
		goto out;

	/* Now truncate and flush to our database. */
	if (NULL != (f = fdopen(fd, "w"))) {
		ical_printfile(f, p);
		rc = 1;
	} else
		perror(NULL);

out:
	/* Release the EXLOCK. */
	if (-1 == flock(fd, LOCK_UN)) 
		perror(file);

	ical_free(p);
	if (NULL != f)
		fclose(f);
	else
		close(fd);
	free(buf);
	return(rc);
}
#endif
