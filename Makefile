.POSIX:

CONFIGFILE = config.mk
include $(CONFIGFILE)

OS = linux
# Linux:   linux
# Mac OS:  macos
# Windows: windows
include mk/$(OS).mk


LIB_MAJOR = 1
LIB_MINOR = 0
LIB_VERSION = $(LIB_MAJOR).$(LIB_MINOR)
LIB_NAME = tracebitmap


OBJ =\
	libtracebitmap_trace.o

HDR =\
	libtracebitmap.h

LOBJ = $(OBJ:.o=.lo)


all: libtracebitmap.a libtracebitmap.$(LIBEXT) demo
$(OBJ): $(HDR)
$(LOBJ): $(HDR)

.c.o:
	$(CC) -c -o $@ $< $(CFLAGS) $(CPPFLAGS)

.c.lo:
	$(CC) -fPIC -c -o $@ $< $(CFLAGS) $(CPPFLAGS)

demo: demo.o libtracebitmap.a
	$(CC) -o $@ demo.o libtracebitmap.a $(CFLAGS) $(CPPFLAGS)

libtracebitmap.a: $(OBJ)
	@rm -f -- $@
	$(AR) rc $@ $(OBJ)

libtracebitmap.$(LIBEXT): $(LOBJ)
	$(CC) $(LIBFLAGS) -o $@ $(LOBJ) $(LDFLAGS)

install: libtracebitmap.a libtracebitmap.$(LIBEXT)
	mkdir -p -- "$(DESTDIR)$(PREFIX)/lib"
	mkdir -p -- "$(DESTDIR)$(PREFIX)/include"
	cp -- libtracebitmap.a "$(DESTDIR)$(PREFIX)/lib/"
	cp -- libtracebitmap.$(LIBEXT) "$(DESTDIR)$(PREFIX)/lib/libtracebitmap.$(LIBMINOREXT)"
	$(FIX_INSTALL_NAME) "$(DESTDIR)$(PREFIX)/lib/libtracebitmap.$(LIBMINOREXT)"
	ln -sf -- libtracebitmap.$(LIBMINOREXT) "$(DESTDIR)$(PREFIX)/lib/libtracebitmap.$(LIBMAJOREXT)"
	ln -sf -- libtracebitmap.$(LIBMAJOREXT) "$(DESTDIR)$(PREFIX)/lib/libtracebitmap.$(LIBEXT)"
	cp -- libtracebitmap.h "$(DESTDIR)$(PREFIX)/include/"

uninstall:
	-rm -f -- "$(DESTDIR)$(PREFIX)/lib/libtracebitmap.a"
	-rm -f -- "$(DESTDIR)$(PREFIX)/lib/libtracebitmap.$(LIBMAJOREXT)"
	-rm -f -- "$(DESTDIR)$(PREFIX)/lib/libtracebitmap.$(LIBMINOREXT)"
	-rm -f -- "$(DESTDIR)$(PREFIX)/lib/libtracebitmap.$(LIBEXT)"
	-rm -f -- "$(DESTDIR)$(PREFIX)/include/libtracebitmap.h"

clean:
	-rm -f -- *.o *.a *.lo *.su *.so *.so.* *.dll *.dylib
	-rm -f -- *.gch *.gcov *.gcno *.gcda *.$(LIBEXT) demo

.SUFFIXES:
.SUFFIXES: .lo .o .c

.PHONY: all install uninstall clean
