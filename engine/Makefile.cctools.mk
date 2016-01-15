# Android CCtools makefile.
# clone nanogl to .. and place libSDL2.so and libm_hard.a here.
CC ?= gcc
CXX ?= g++
CFLAGS = -Og -ggdb -mfloat-abi=hard -mhard-float -mthumb -mfpu=neon -mcpu=cortex-a9 -pipe -mvectorize-with-neon-quad -D__ANDROID__ -DVECTORIZE_SINCOS -DSOFTFP_LINK -DLOAD_HARDFP -fPIC -fsigned-char -funsafe-math-optimizations -ftree-vectorize -fgraphite-identity -floop-interchange -funsafe-loop-optimizations -finline-limit=1024

LDFLAGS =
LIBS = -lm
LBITS := $(shell getconf LONG_BIT)
ifeq ($(LBITS),64)
		  LDFLAGS += -m32
		  CFLAGS += -m32
endif
TOPDIR = $(PWD)/..
XASHXT ?= $(TOPDIR)/../xashXT
HLSDK ?= $(TOPDIR)/../halflife/
INCLUDES = -I/usr/include/SDL2 -Icommon -I../common -I. -I../pm_shared -Iclient -I$(XASHXT)/common -DXASH_GLES -DVECTORIZE_SINCOS -D_NDK_MATH_NO_SOFTFP=1 -I$(HLSDK)/public -Iserver -Iclient/vgui -Icommon/sdl -I../SDL2/include -I../nanogl -I../nanogl/GL
DEFINES =
ifeq ($(XASH_DEDICATED),1)
		  DEFINES += -DXASH_DEDICATED
else
		  DEFINES += -DXASH_SDL
		  LIBS += libSDL2.so
endif

# Some libc implementations cannot use libdl, so disable it by default
ifeq ($(XASH_STATIC),1)

		  ifneq ($(XASH_STATIC_LIBDL),1)
	DEFINES += -DNO_LIBDL
		  endif

		  XASH_SINGLE_BINARY := 1
endif

ifneq ($(XASH_STATIC),1)
		  LIBS +=
endif

ifeq ($(XASH_STATIC_LIBDL),1)
		  LIBS += -ldl
endif
ifeq ($(XASH_DLL_LOADER),1)
		  DEFINES += -DDLL_LOADER
		  ifeq ($(XASH_SINGLE_BINARY),1)
	LIBS += libloader.a -pthread -lm
		  else
	LIBS += libloader.so
		  endif
endif

ifeq ($(XASH_SINGLE_BINARY),1)
		  DEFINES += -DSINGLE_BINARY
endif

ifeq ($(XASH_VGUI),1)
#    INCLUDES += -I$(HLSDK)/utils/vgui/include/
		  DEFINES += -DXASH_VGUI
#    LIBS += vgui.so
endif

%.o : %.c
	$(CC) $(CFLAGS) $(INCLUDES) $(DEFINES) -fPIC -c $< -o $@

%.o : %.cpp
	$(CXX) $(CFLAGS) $(INCLUDES) $(DEFINES) -fPIC -c $< -o $@

SRCS_CPP = $(wildcard platform/android/*.cpp) $(wildcard ../nanogl/*.cpp)
#ifeq ($(XASH_VGUI),1)
#    SRCS_CPP += $(wildcard client/vgui/*.cpp)
#endif

OBJS_CPP = $(SRCS_CPP:.cpp=.o)
SRCS = $(wildcard server/*.c) $(wildcard client/*.c) $(wildcard platform/android/*.c) $(wildcard client/vgui/*.c) $(wildcard common/*.c) $(wildcard common/imagelib/*.c) $(wildcard common/soundlib/*.c) $(wildcard common/soundlib/libmpg/*.c) $(wildcard ./common/sdl/*.c)
OBJS = $(SRCS:.c=.o)

libxash.so : $(OBJS) $(OBJS_CPP)
ifeq ($(XASH_DLL_LOADER),1)
	cp $(TOPDIR)/loader/libloader.so .
endif
	g++ -o libxash.so $(LDFLAGS) -shared libm_hard.a $(OBJS) $(OBJS_CPP) $(LIBS) -Wl,--no-warn-mismatch -lstdc++ -llog -Wl,-soname=libxash.so

ifeq ($(XASH_SINGLE_BINARY),1)
xash: $(OBJS) $(OBJS_CPP)

		  ifeq ($(XASH_STATIC),1)
	$(CC) -o xash -static $(LDFLAGS) $(OBJS) $(OBJS_CPP) $(LIBS)
		  else
	$(CC) -o xash -fvisibility=hidden $(LDFLAGS) $(OBJS) $(OBJS_CPP) $(LIBS)
		  endif

endif

.PHONY: depend clean list

clean:
	$(RM) $(OBJS) $(OBJS_CPP) libxash.so xash

list:
	@echo Sources:
	@echo $(SRCS)
	@echo C++ Sources:
	@echo $(SRCS_CPP)
	@echo Objects:
	@echo $(OBJS) $(OBJS_CPP)

depend: $(SRCS) $(SRCS_CPP)
	touch Makefile.dep
	makedepend -fMakefile.dep -- $(DEFINES) -Icommon -I../common -I. -I../pm_shared -Iclient -Iserver -Iclient/vgui -- $^

include Makefile.dep
