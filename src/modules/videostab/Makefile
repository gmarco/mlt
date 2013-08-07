include ../../../config.mak

CFLAGS += -I../.. -I$(shell pwd)/vid.stab/src/ -I /usr/include/orc-0.4 -ggdb -O2 -g
ifdef SSE2_FLAGS
CFLAGS += -msse2
endif

LDFLAGS += -L../../framework -lmlt -lm -lorc-0.4
VSFILES= $(shell find vid.stab/src -name '*.c')

TARGET = ../libmltvideostab$(LIBSUF)

OBJS = factory.o \
	   filter_videostab.o filter_videostab2.o \
		stab/klt/convolve.o           stab/klt/klt.o                stab/klt/pyramid.o            stab/klt/trackFeatures.o \
		stab/klt/error.o              stab/klt/klt_util.o           stab/klt/selectGoodFeatures.o \
		stab/estimate.o    stab/resample.o stab/utils.o    stab/vector.o stabilize.o \
		vid.stab/src/transformtype.o vid.stab/src/transformfixedpoint.o transform_image.o
#OLD STAB
#OBJS+=stabilize.o  transform.o tlist.o

OBJS+=	vid.stab/src/orc/motiondetectorc.o vid.stab/src/boxblur.o \
		vid.stab/src/motiondetect.o vid.stab/src/vsvector.o  \
		vid.stab/src/transform.o vid.stab/src/motiondetect_opt.o \
		vid.stab/src/libvidstab.o vid.stab/src/frameinfo.o tlist.o \
		vid.stab/src/localmotion2transform.o
		#vid.stab/src/libdeshake.o tlist.o

#$(patsubst %.c,%.o,$(VSFILES))
		
		


SRCS := $(OBJS:.o=.c)

all: 	$(TARGET)

$(TARGET): $(OBJS)
		$(CC) $(SHFLAGS) -o $@ $(OBJS) $(LDFLAGS)

depend:	$(SRCS)
		$(CC) -MM $(CFLAGS) $^ 1>.depend

distclean:	clean
		rm -f .depend

clean:	
		rm -f $(OBJS) $(TARGET)

install: all
	install -m 755 $(TARGET) "$(DESTDIR)$(moduledir)"
	install -d $(DESTDIR)$(mltdatadir)/videostab
	install -m 644 *.yml "$(DESTDIR)$(mltdatadir)/videostab"


ifneq ($(wildcard .depend),)
include .depend
endif
