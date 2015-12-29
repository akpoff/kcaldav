.SUFFIXES: .3 .3.html .5 .8 .5.html .8.html .1 .1.html

# This is the directory prepended to all calendar requests.
# It is relative to the CGI process's file-system root.
# It contains top-level processing information (like the principal
# password file) and miscellaneous data files to be served.
# An example follows:
#CALDIR		 = /caldav
# This is the URI of the static (CSS, JS) files.
#HTDOCS	 	 = /
# This is the file-system directory of CALDIR.
#CALPREFIX	 = /var/www/caldav
# This is the file-system directory of the CGI script.
#CGIPREFIX	 = /var/www/cgi-bin/caldav
# This is the file-system directory of HTDOCS.
#HTDOCSPREFIX	 = /var/www/htdocs
# This is the file-system root for system programs and manpages.
#PREFIX		 = /usr/local

# Use this for installing into a single directory.
# I use this on my Mac OS X laptop (no chroot(2)).
CALDIR		 = /Users/kristaps/Sites/kcaldav
HTDOCS	 	 = /~kristaps/kcaldav
CALPREFIX	 = /Users/kristaps/Sites/kcaldav
CGIPREFIX	 = /Users/kristaps/Sites/kcaldav
HTDOCSPREFIX	 = /Users/kristaps/Sites/kcaldav
PREFIX		 = /usr/local

# ...and this for deployment on BSD.lv, which has its static files in a
# virtual host and runs within a chroot(2).
#CALDIR		 = /caldav
#HTDOCS	 	 = /kcaldav
#CALPREFIX	 = /var/www/caldav
#CGIPREFIX	 = /var/www/cgi-bin
#HTDOCSPREFIX	 = /var/www/vhosts/www.bsd.lv/htdocs/kcaldav
#PREFIX		 = /usr/local

# Add any special dependency directives here.
# The -D LOGTIMESTAMP directive instructs the logger to log a timestamp
# next to the date.
# Most web servers provide this; others (e.g., OpenBSD httpd(8)) don't.

#### For OpenBSD:
LIBS		 = -lexpat -lm -lsqlite3
STATIC		 = -static
CPPFLAGS	+= -I/usr/local/include -DLOGTIMESTAMP=1 
BINLDFLAGS	 = -L/usr/local/lib

#### For Mac OS X:
#LIBS		 = -lexpat -lsqlite3
#STATIC		 = 
#CPPFLAGS	+= -I/usr/local/include 
#BINLDFLAGS	 = -L/usr/local/lib

#### For Linux:
#LIBS		 = -lexpat -lbsd -lm -lsqlite3
#STATIC		 = 
#CPPFLAGS	+= -I/usr/local/include 
#BINLDFLAGS	 = -L/usr/local/lib

# You probably don't want to change anything after this point.

BINLIBS		 = -lkcgi -lkcgixml -lkcgihtml -lz $(LIBS) 
BINS		 = kcaldav \
		   kcaldav.passwd \
		   test-caldav \
		   test-ical \
		   test-nonce \
		   test-rrule
TESTSRCS 	 = test-caldav.c \
		   test-ical.c \
		   test-nonce.c \
		   test-rrule.c
TESTOBJS 	 = test-caldav.o \
		   test-ical.o \
		   test-nonce.o \
		   test-rrule.o
HTMLS	 	 = index.html \
		   kcaldav.8.html \
		   kcaldav.passwd.1.html \
		   libkcaldav.3.html
MANS		 = kcaldav.in.8 \
		   kcaldav.passwd.in.1 \
		   libkcaldav.3
CTESTSRCS	 = test-explicit_bzero.c \
		   test-memmem.c \
		   test-reallocarray.c
ALLSRCS		 = Makefile \
		   $(TESTSRCS) \
		   $(CTESTSRCS) \
		   $(MANS) \
		   buf.c \
		   caldav.c \
		   collection.html \
		   compat-explicit_bzero.c \
		   compat-memmem.c \
		   compat-reallocarray.c \
		   configure \
		   config.h.post \
		   config.h.pre \
		   datetime.c \
		   db.c \
		   delete.c \
		   dynamic.c \
		   err.c \
		   extern.h \
		   get.c \
		   home.html \
		   ical.c \
		   kcaldav.c \
		   kcaldav.h \
		   kcaldav.passwd.c \
		   libkcaldav.h \
		   md5.js \
		   md5.c \
		   md5.h \
		   options.c \
		   principal.c \
		   propfind.c \
		   property.c \
		   proppatch.c \
		   put.c \
		   resource.c \
		   style.css \
		   util.c
LIBOBJS		 = buf.o \
		   caldav.o \
		   compat-explicit_bzero.o \
		   compat-memmem.o \
		   compat-reallocarray.o \
		   datetime.o \
		   err.o \
		   ical.o
OBJS		 = db.o \
		   principal.o \
		   resource.o
BINOBJS		 = delete.o \
		   dynamic.o \
		   get.o \
		   kcaldav.o \
		   md5.o \
		   options.o \
		   propfind.o \
		   property.o \
		   proppatch.o \
		   put.o \
		   util.o
ALLOBJS		 = $(TESTOBJS) \
		   $(LIBOBJS) \
		   $(BINOBJS) \
		   $(OBJS) \
		   kcaldav.passwd.o
VERSIONS	 = version_0_0_4.xml \
		   version_0_0_5.xml \
		   version_0_0_6.xml \
		   version_0_0_7.xml \
		   version_0_0_8.xml \
		   version_0_0_9.xml \
		   version_0_0_10.xml \
		   version_0_0_11.xml \
		   version_0_0_12.xml \
		   version_0_0_13.xml \
		   version_0_0_14.xml \
		   version_0_0_15.xml \
		   version_0_0_16.xml \
		   version_0_1_0.xml \
		   version_0_1_1.xml \
		   version_0_1_2.xml \
		   version_0_1_3.xml
VERSION		 = 0.1.2
CFLAGS 		+= -g -W -Wall -Wstrict-prototypes -Wno-unused-parameter -Wwrite-strings
CFLAGS		+= -DCALDIR=\"$(CALDIR)\"
CFLAGS		+= -DHTDOCS=\"$(HTDOCS)\"
CFLAGS		+= -DVERSION=\"$(VERSION)\"

all: $(BINS) kcaldav.8 kcaldav.passwd.1

www: kcaldav.tgz kcaldav.tgz.sha512 $(HTMLS)

afl: all
	install -m 0555 test-ical afl/test-ical
	install -m 0555 test-caldav afl/test-caldav

config.h: config.h.pre config.h.post configure $(CTESTSRCS)
	rm -f config.log
	CC="$(CC)" CFLAGS="$(CFLAGS)" ./configure

installcgi: all
	mkdir -p $(CGIPREFIX)
	mkdir -p $(HTDOCSPREFIX)
	mkdir -p $(CALPREFIX)
	install -m 0555 kcaldav $(CGIPREFIX)
	install -m 0555 kcaldav $(CGIPREFIX)/kcaldav.cgi
	install -m 0444 md5.js style.css $(HTDOCSPREFIX)
	install -m 0444 home.html collection.html $(CALPREFIX)

install: all
	mkdir -p $(PREFIX)/bin
	mkdir -p $(PREFIX)/lib
	mkdir -p $(PREFIX)/include
	mkdir -p $(PREFIX)/man/man8
	mkdir -p $(PREFIX)/man/man3
	mkdir -p $(PREFIX)/man/man1
	install -m 0555 kcaldav.passwd $(PREFIX)/bin
	install -m 0444 kcaldav.passwd.1 $(PREFIX)/man/man1
	install -m 0444 kcaldav.8 $(PREFIX)/man/man8
	install -m 0444 libkcaldav.3 $(PREFIX)/man/man3
	install -m 0444 libkcaldav.a $(PREFIX)/lib
	install -m 0444 libkcaldav.h $(PREFIX)/include

installwww: www
	mkdir -p $(PREFIX)/snapshots
	install -m 0444 index.css $(HTMLS) $(PREFIX)
	install -m 0444 kcaldav.tgz kcaldav.tgz.sha512 $(PREFIX)/snapshots/
	install -m 0444 kcaldav.tgz $(PREFIX)/snapshots/kcaldav-$(VERSION).tgz
	install -m 0444 kcaldav.tgz.sha512 $(PREFIX)/snapshots/kcaldav-$(VERSION).tgz.sha512

kcaldav.tgz.sha512: kcaldav.tgz
	openssl dgst -sha512 kcaldav.tgz >$@

kcaldav.tgz:
	mkdir -p .dist/kcaldav-$(VERSION)
	cp $(ALLSRCS) .dist/kcaldav-$(VERSION)
	(cd .dist && tar zcf ../$@ kcaldav-$(VERSION))
	rm -rf .dist

libkcaldav.a: $(LIBOBJS)
	$(AR) -rs $@ $(LIBOBJS)

kcaldav: $(BINOBJS) $(OBJS) libkcaldav.a
	$(CC) $(BINCFLAGS) -o $@ $(STATIC) $(BINOBJS) $(OBJS) libkcaldav.a $(BINLDFLAGS) $(BINLIBS) 

kcaldav.passwd: kcaldav.passwd.o md5.o $(OBJS) libkcaldav.a
	$(CC) -o $@ kcaldav.passwd.o md5.o $(OBJS) libkcaldav.a $(LIBS)

test-ical: test-ical.o libkcaldav.a
	$(CC) -o $@ test-ical.o libkcaldav.a $(LIBS)

test-rrule: test-rrule.o libkcaldav.a
	$(CC) -o $@ test-rrule.o libkcaldav.a $(LIBS)

test-nonce: test-nonce.o $(OBJS) libkcaldav.a
	$(CC) -o $@ test-nonce.o $(OBJS) libkcaldav.a $(LIBS)

test-caldav: test-caldav.o libkcaldav.a
	$(CC) -o $@ test-caldav.o libkcaldav.a $(LIBS)

$(ALLOBJS): extern.h libkcaldav.h md5.h config.h

$(BINOBJS): kcaldav.h

index.html: index.xml $(VERSIONS)
	sblg -t index.xml -o- $(VERSIONS) | sed "s!@VERSION@!$(VERSION)!g" >$@

kcaldav.8: kcaldav.in.8
	sed -e "s!@CALDIR@!$(CALDIR)!g" \
	    -e "s!@PREFIX@!$(PREFIX)!g" kcaldav.in.8 >$@

kcaldav.passwd.1: kcaldav.passwd.in.1
	sed -e "s!@CALDIR@!$(CALDIR)!g" \
	    -e "s!@PREFIX@!$(PREFIX)!g" kcaldav.passwd.in.1 >$@

clean:
	rm -f $(ALLOBJS) $(BINS) kcaldav.8 kcaldav.passwd.1 libkcaldav.a
	rm -rf *.dSYM
	rm -f $(HTMLS) kcaldav.tgz kcaldav.tgz.sha512
	rm -f test-memmem test-reallocarray test-explicit_bzero
	rm -f config.h config.log

.8.8.html .5.5.html .3.3.html .1.1.html:
	mandoc -Wall -Thtml $< >$@
