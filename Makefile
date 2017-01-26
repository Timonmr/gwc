prefix = /usr/local
exec_prefix = ${prefix}

BINDIR = $(DESTDIR)/${exec_prefix}/bin
DATADIR = $(DESTDIR)/${datarootdir}
DATAROOTDIR = $(DESTDIR)/${prefix}/share
LIBDIR = $(DESTDIR)/${exec_prefix}/lib
APPNAME = gnome_wave_cleaner
GNOME = /usr

pixmapdir = $(DATADIR)/pixmaps
HELPDIR = $(DESTDIR)/$(GNOME)/share/gnome/help/$(APPNAME)
HELPDIRC = $(HELPDIR)/C
DOCDIR = $(DATADIR)/doc/gwc

# use these entries for SuSE and maybe other distros
#DOCDIR = /usr/share/doc/packages/gwc
#HELPDIR = $(DOCDIR)
#HELPDIRC = $(DOCDIR)

# Where the user preferences for gwc are stored (~user/.gnome/...)
CONFIGDIR = /$(APPNAME)/config/

DEFS = -DDATADIR=\"$(DATADIR)\" -DLIBDIR=\"$(LIBDIR)\" -DAPPNAME=\"$(APPNAME)\" -DHAVE_ALSA  -DHAVE_FFTW3 -DFFTWPREC=2  
CFLAGS = -D_FILE_OFFSET_BITS=64 -Wall -g -O6 -D_REENTRANT -DORBIT2=1 -pthread -I/usr/include/libgnomeui-2.0 -I/usr/include/gnome-keyring-1 -I/usr/include/libbonoboui-2.0 -I/usr/include/libxml2 -I/usr/include/libgnome-2.0 -I/usr/include/libbonobo-2.0 -I/usr/include/bonobo-activation-2.0 -I/usr/include/orbit-2.0 -I/usr/include/libgnomecanvas-2.0 -I/usr/include/gail-1.0 -I/usr/include/libart-2.0 -I/usr/include/gtk-2.0 -I/usr/lib/x86_64-linux-gnu/gtk-2.0/include -I/usr/include/gio-unix-2.0/ -I/usr/include/cairo -I/usr/include/pango-1.0 -I/usr/include/atk-1.0 -I/usr/include/cairo -I/usr/include/pixman-1 -I/usr/include/libpng16 -I/usr/include/pango-1.0 -I/usr/include/harfbuzz -I/usr/include/pango-1.0 -I/usr/include/freetype2 -I/usr/include/gdk-pixbuf-2.0 -I/usr/include/libpng16 -I/usr/include/gnome-vfs-2.0 -I/usr/lib/x86_64-linux-gnu/gnome-vfs-2.0/include -I/usr/include/gconf/2 -I/usr/include/dbus-1.0 -I/usr/lib/x86_64-linux-gnu/dbus-1.0/include -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include 

LIBS= meschach.a -lgnomeui-2 -lSM -lICE -lbonoboui-2 -lgnome-2 -lpopt -lbonobo-2 -lbonobo-activation -lORBit-2 -lgnomecanvas-2 -lart_lgpl_2 -lgtk-x11-2.0 -lgdk-x11-2.0 -lpangocairo-1.0 -latk-1.0 -lcairo -lgio-2.0 -lpangoft2-1.0 -lpango-1.0 -lfontconfig -lfreetype -lgdk_pixbuf-2.0 -lgnomevfs-2 -lgconf-2 -lgthread-2.0 -pthread -lgmodule-2.0 -pthread -lgobject-2.0 -lglib-2.0 -lsndfile -lasound  -lfftw3   -lm

SRC = tap_reverb_file_io.c tap_reverb.c reverb.c dialog.c gwc.c audio_device.c audio_edit.c audio_util.c gtkled.c gtkledbar.c preferences.c drawing.c amplify.c denoise.c undo.c declick.c sample_block.c decrackle.c stat.c dethunk.c i0.c i1.c chbevl.c markers.c encode.c soundfile.c pinknoise.c biquad.c
OBJS = $(SRC:.c=.o)

BINFILES = gwc
pixmap_DATA = gwc-logo.png
DOCFILES = README INSTALL TODO COPYING Changelog
HELPFILES = gwc_qs.html gwc.html topic.dat
HELPFILESSRCD = doc/C

###
CC = gcc
COMPILE = $(CC) $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS)

### handy to have around for checking buffer overruns
#EFENCE = -lefence
EFENCE =

all : gwc

gwc : $(OBJS) meschach.a
	$(CC) $(LDFLAGS) $(OBJS) $(EFENCE) $(LFLAGS) $(LIBS) -o gwc

audio_device.o : audio_device.c audio_alsa.c audio_oss.c audio_osx.c audio_pa.c Makefile
	$(COMPILE) -c audio_device.c

.c.o :
	$(COMPILE) -c $<

install : gwc
	install -d $(BINDIR)
	install -d $(DOCDIR)
	install -d $(pixmapdir)
	install -d $(HELPDIRC)
	install -p $(BINFILES) $(BINDIR)/$(APPNAME)
	install -p -m 0644 $(DOCFILES) $(DOCDIR)
	for hf in $(HELPFILES) ; do install -p -m 0644 $(HELPFILESSRCD)/$$hf $(HELPDIRC) ; done
	install -p -m 0644 $(pixmap_DATA) $(pixmapdir)

uninstall :
	( cd $(BINDIR) && rm -f $(BINFILES) )
	( cd $(DOCDIR) && rm -f $(DOCFILES) )
	( cd $(HELPDIRC) && rm -f $(HELPFILES) )
	( cd $(pixmapdir) && rm -f $(pixmap_DATA) )
	( rmdir --ignore-fail-on-non-empty $(DOCDIR) $(HELPDIRC) $(HELPDIR) $(pixmapdir) )

meschach.a : meschach/meschach.a
	cp meschach/meschach.a .

meschach/meschach.a :
	(cd meschach ; ./configure --with-sparse ; make part1 ; make part2 ; make part3 ; cp machine.h ..)

clean :
	rm -f gwc *.o core meschach.a meschach/meschach.a
	(cd meschach ; make realclean)