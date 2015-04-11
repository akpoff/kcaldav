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
#include <sys/mman.h>

#include <assert.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <kcgi.h>
#include <kcgixml.h>

#include "extern.h"

#define	MAXNONCES 	1000
#define	RECORDSZ	24
#define NONCESZ	 	16

/*
 * Given a 32-byte nonce value "nonce", look in our database of nonces
 * for that nonce and its count.
 * The count that we're given must be greater than what we have.
 * We then set the count to be the new count.
 * Returns -1 on failure, 1 if we found the nonce and the nounce count
 * is ok, or 0 if we didn't find the nonce or it had a bad count.
 */
int
nonce_verify(const char *fname, const char *nonce, size_t count)
{
	struct stat	 st;
	char		*map;
	off_t		 pos;
	int		 fd;

	assert(NONCESZ == strlen(nonce));

	/* Open nonce file, then mmap() it. */
	if (-1 == (fd = open_lock_ex(fname, O_RDWR|O_CREAT, 0600)))
		return(-1);

	if (-1 == fstat(fd, &st)) {
		kerr("%s: fstat", fname);
		close_unlock(fname, fd);
		return(-1);
	} else if (st.st_size % RECORDSZ) {
		kerrx("%s: bad nonce filesize", fname);
		close_unlock(fname, fd);
		return(-1);
	} else if (0 == st.st_size) {
		close_unlock(fname, fd);
		return(0);
	}

	map = mmap(NULL, st.st_size, 
		PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

	if (MAP_FAILED == map) {
		kerr("%s: mmap", fname);
		close_unlock(fname, fd);
		return(-1);
	}

	/* Look for our nonce. */
	for (pos = 0; pos < st.st_size; pos += RECORDSZ) {
		if (memcmp(nonce, &map[pos], NONCESZ))
			continue;
		/* Check whether the nonce is replayed. */
		if (count <= *(uint32_t *)&map[pos + NONCESZ]) {
			close_unlock(fname, fd);
			return(0);
		}
		count++;
		memcpy(&map[pos + NONCESZ], &count, 4);
		break;
	}

	close_unlock(fname, fd);
	return(pos < st.st_size);
}

int
nonce_new(const char *fname, char **np)
{
	static char	 nonce[NONCESZ + 1];
	int		 fd;
	struct stat	 st;
	off_t		 pos, sz, oldestpos;
	size_t		 i;
	char		*map;
	uint32_t	 cur, oldest;

	*np = nonce;

	/* Open nonce file, then mmap() it. */
	if (-1 == (fd = open_lock_ex(fname, O_RDWR|O_CREAT, 0600)))
		return(-1);

	if (-1 == fstat(fd, &st)) {
		kerr("%s: fstat", fname);
		close_unlock(fname, fd);
		return(-1);
	} else if (st.st_size % RECORDSZ) {
		kerrx("%s: bad nonce filesize", fname);
		close_unlock(fname, fd);
		return(-1);
	}

	/* Should we enlarge the file? */
	sz = st.st_size;
	if (st.st_size < MAXNONCES * RECORDSZ) {
		st.st_size += RECORDSZ;
		if (-1 == ftruncate(fd, st.st_size)) {
			kerr("%s: ftruncate", fname);
			close_unlock(fname, fd);
			return(-1);
		}
	}

	map = mmap(NULL, st.st_size, 
		PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

	if (MAP_FAILED == map) {
		kerr("%s: mmap", fname);
		close_unlock(fname, fd);
		return(-1);
	}

	/*
	 * Create a random nonce then look for it.
	 * Enforce that it's random within the file.
	 */
	do {
		for (i = 0; i < sizeof(nonce) - 1; i++)
			snprintf(nonce + i, 2, "%01X", 
				arc4random_uniform(128));
		for (pos = 0; pos < sz; pos += RECORDSZ)
			if (0 == memcmp(&map[pos], nonce, NONCESZ))
				break;
	} while (pos < sz);

	/* 
	 * If we're at the maximum nonce file-size, then find the oldest
	 * nonce value and evict it.
	 * Otherwise, use the last part of the file.
	 */
	if (sz == st.st_size) {
		oldest = UINT32_MAX;
		oldestpos = 0;
		for (pos = 0; pos < sz; pos += RECORDSZ) {
			cur = *(uint32_t *)&map[pos + RECORDSZ + 4];
			if (cur < oldest) {
				oldest = cur;
				oldestpos = pos;
			}
		}
		assert(oldest != UINT32_MAX);
		pos = oldestpos;
	} else 
		pos = sz;

	memcpy(&map[pos], nonce, NONCESZ);
	*(uint32_t *)&map[pos + NONCESZ] = 0;
	*(uint32_t *)&map[pos + NONCESZ + 4] = time(NULL);
	close_unlock(fname, fd);
	return(1);
}
