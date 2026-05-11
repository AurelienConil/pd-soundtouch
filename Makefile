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

# ---- SoundTouch static lib (built before linking the external) ----

SOUNDTOUCH_SRCS = \
    SoundTouch.cpp TDStretch.cpp RateTransposer.cpp FIFOSampleBuffer.cpp \
    AAFilter.cpp FIRFilter.cpp InterpolateCubic.cpp InterpolateLinear.cpp \
    InterpolateShannon.cpp PeakFinder.cpp cpu_detect_x86.cpp \
    sse_optimized.cpp mmx_optimized.cpp

SOUNDTOUCH_OBJS = $(addprefix $(SOUNDTOUCH_SRC)/, $(SOUNDTOUCH_SRCS:.cpp=.o))

ST_CXXFLAGS = -I$(SOUNDTOUCH_INC) -I$(SOUNDTOUCH_INC2) -I$(SOUNDTOUCH_SRC) -O2 -fPIC

$(SOUNDTOUCH_SRC)/%.o: $(SOUNDTOUCH_SRC)/%.cpp
	$(CXX) $(ST_CXXFLAGS) -c $< -o $@

soundtouch.a: $(SOUNDTOUCH_OBJS)
	$(AR) rcs $@ $^

# Make the object file depend on soundtouch.a so it's built before linking
pitchshift~.$(object.extension): soundtouch.a
