.SUFFIXES: .5 .8 .5.html .8.html

# You'll absolutely want to change this. 
# It is the directory prepended to all calendar requests.
# It should not end in a trailing slash.
CALDIR		 = /tmp

# You probably don't want to change anything after this.
BINS		 = kcaldav \
		   test-auth \
		   test-caldav \
		   test-config \
		   test-ical \
		   test-prncpl
TESTSRCS 	 = test-auth.c \
		   test-caldav.c \
		   test-config.c \
		   test-ical.c \
		   test-prncpl.c
TESTOBJS 	 = test-auth.o \
		   test-caldav.o \
		   test-config.o \
		   test-ical.o \
		   test-prncpl.o
MANS		 = kcaldav.8 \
		   kcaldav.conf.5 \
		   kcaldav.passwd.5
HTMLS	 	 = index.html \
		   kcaldav.8.html \
		   kcaldav.conf.5.html \
		   kcaldav.passwd.5.html
MANS		 = kcaldav.8 \
		   kcaldav.conf.5 \
		   kcaldav.passwd.5
ALLSRCS		 = Makefile \
		   $(TESTSRCS) \
		   $(MANS) \
		   auth.c \
		   buf.c \
		   caldav.c \
		   config.c \
		   ical.c \
		   main.c \
		   md5.c \
		   principal.c 
OBJS		 = auth.o \
		   buf.o \
		   caldav.o \
		   config.o \
		   ical.o \
		   md5.o \
		   principal.o
ALLOBJS		 = $(TESTOBJS) \
		   main.o \
		   $(OBJS)
VERSIONS	 = version_0_0_4.xml
VERSION		 = 0.0.4
CFLAGS 		+= -g -W -Wall -Wstrict-prototypes -Wno-unused-parameter -Wwrite-strings
CFLAGS		+= -DCALDIR=\"$(CALDIR)\"

all: $(BINS)

www: kcaldav.tgz kcaldav.tgz.sha512 $(HTMLS)

install: all
	mkdir -p $(PREFIX)
	install -m 0755 kcaldav $(PREFIX)/kcaldav.cgi

installwww: www
	mkdir -p $(PREFIX)/snapshots
	install -m 0444 index.html index.css $(HTMLS) $(PREFIX)
	install -m 0444 sample.c $(PREFIX)/sample.c.txt
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

kcaldav: main.o $(OBJS)
	$(CC) -o $@ main.o $(OBJS) -lkcgi -lkcgixml -lexpat -lz

test-ical: test-ical.o $(OBJS)
	$(CC) -o $@ test-ical.o $(OBJS) -lexpat

test-auth: test-auth.o $(OBJS)
	$(CC) -o $@ test-auth.o $(OBJS) -lexpat

test-caldav: test-caldav.o $(OBJS)
	$(CC) -o $@ test-caldav.o $(OBJS) -lexpat

test-config: test-config.o $(OBJS)
	$(CC) -o $@ test-config.o $(OBJS) -lexpat

test-prncpl: test-prncpl.o $(OBJS)
	$(CC) -o $@ test-prncpl.o $(OBJS) -lexpat

$(ALLOBJS): extern.h md5.h

index.html: index.xml $(VERSIONS)
	sblg -t index.xml -o- $(VERSIONS) | sed "s!@VERSION@!$(VERSION)!g" >$@

clean:
	rm -f $(ALLOBJS) $(BINS)
	rm -rf *.dSYM
	rm -f $(HTMLS) kcaldav.tgz kcaldav.tgz.sha512

.8.8.html .5.5.html:
	mandoc -Thtml $< >$@

