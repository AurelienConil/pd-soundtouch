# Makefile for pitchshift~ — Pure Data external using SoundTouch

lib.name = pitchshift~

SOUNDTOUCH_SRC  = soundtouch/source/SoundTouch
SOUNDTOUCH_INC  = soundtouch/include
SOUNDTOUCH_INC2 = soundtouch/include/soundtouch

class.sources = pitchshift~.cpp

cflags  = -I$(SOUNDTOUCH_INC) -I$(SOUNDTOUCH_INC2)
ldlibs  = soundtouch.a -lm -lstdc++

datafiles =

PDLIBBUILDER_DIR = pd-lib-builder/
include $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder

# ---- Architecture detection ----

UNAME_M := $(shell uname -m)

ifeq ($(findstring arm,$(UNAME_M)),arm)
  ARCH_ARM = 1
endif
ifeq ($(UNAME_M),aarch64)
  ARCH_ARM = 1
endif

# ---- SoundTouch static lib (built before linking the external) ----

SOUNDTOUCH_SRCS_COMMON = \
    SoundTouch.cpp TDStretch.cpp RateTransposer.cpp FIFOSampleBuffer.cpp \
    AAFilter.cpp FIRFilter.cpp InterpolateCubic.cpp InterpolateLinear.cpp \
    InterpolateShannon.cpp PeakFinder.cpp

ifdef ARCH_ARM
  # ARM: no x86 SIMD files; use -O2 -mfpu=neon for NEON auto-vectorization
  SOUNDTOUCH_SRCS  = $(SOUNDTOUCH_SRCS_COMMON)
  ST_ARCH_FLAGS    = -O2 -mfpu=neon -mfloat-abi=hard
else
  # x86: include SSE/MMX optimized files
  SOUNDTOUCH_SRCS  = $(SOUNDTOUCH_SRCS_COMMON) cpu_detect_x86.cpp \
                     sse_optimized.cpp mmx_optimized.cpp
  ST_ARCH_FLAGS    = -O2
endif

SOUNDTOUCH_OBJS = $(addprefix $(SOUNDTOUCH_SRC)/, $(SOUNDTOUCH_SRCS:.cpp=.o))

ST_CXXFLAGS = -I$(SOUNDTOUCH_INC) -I$(SOUNDTOUCH_INC2) -I$(SOUNDTOUCH_SRC) $(ST_ARCH_FLAGS) -fPIC

$(SOUNDTOUCH_SRC)/%.o: $(SOUNDTOUCH_SRC)/%.cpp
	$(CXX) $(ST_CXXFLAGS) -c $< -o $@

soundtouch.a: $(SOUNDTOUCH_OBJS)
	$(AR) rcs $@ $^

# Make the object file depend on soundtouch.a so it's built before linking
pitchshift~.$(object.extension): soundtouch.a
