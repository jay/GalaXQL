default: all

ifeq "$(shell wx-config --basename)" "wx_msw"

# Compile with MinGW
TARGET_PREFIX=i686-pc-mingw32-
CC  = $(TARGET_PREFIX)gcc
CXX = $(TARGET_PREFIX)g++

# Use static libraries to avoid any dependencies.
PLATFORM_LDFLAGS = -static-libgcc -static-libstdc++

# Use the RC file so that the executable has an icon
%-rc.o: %.rc
	windres -i$< -o$@ $(shell wx-config --cflags)

rcobj=app-rc.o

EXECUTABLE_EXTENSION=.exe

else

PLATFORM_LDFLAGS = -lGLU

endif

MAJOR=2
MINOR=1

sqlite-src    = sqlite3.c
galaxql-src   = RegenerationDialog.cpp galaxql.cpp sqlqueryctrl.cpp
objs          = $(galaxql-src:.cpp=.o)   $(sqlite-src:.c=.o)   $(rcobj)
objs-d        = $(galaxql-src:.cpp=-d.o) $(sqlite-src:.c=-d.o) $(rcobj)

galaxql-exe   = galaxql$(EXECUTABLE_EXTENSION)
galaxql-d-exe = galaxql-d$(EXECUTABLE_EXTENSION)
src-tarball   = galaxql-src-$(MAJOR).$(MINOR).tar.gz
src-zip       = galaxql_$(MAJOR)$(MINOR)_src.zip
bin-zip       = galaxql_$(MAJOR)$(MINOR).zip

WXCXXFLAGS := $(shell wx-config --cflags)
WXLDFLAGS  := $(shell wx-config --libs core,base,gl,html,stc)
CXXFLAGS    = $(WXCXXFLAGS) -O3
CFLAGS      = -O3
DBG_CFLAGS  = -O0 -g -ggdb


%-d.o : %.cpp
	$(CXX) -c $(DBG_CFLAGS) -o $@ $< $(WXCXXFLAGS)

%-d.o : %.c
	$(CC) -c $(DBG_CFLAGS) -o $@ $<

$(galaxql-exe): $(objs)
	$(CXX) -o $@ $^ $(WXLDFLAGS) $(PLATFORM_LDFLAGS)
	strip $@

$(galaxql-d-exe): $(objs-d)
	$(CXX) -o $@ $^ $(WXLDFLAGS) $(PLATFORM_LDFLAGS)

# ZIP the contents of data
#   zip options:
#   -j      -- don't include the directory name "data" in the file paths
#   -9      -- compress at the highest level (best compression)
#   $@      -- the name of the output ZIP file
#   $^      -- the names of the files to include
galaxql.dat: $(wildcard data/*)
	zip -j -9 $@ $^


intermediates=$(galaxql-exe) $(galaxql-d-exe) galaxql.dat galaxql.db $(src-zip) $(src-tarball) $(bin-zip) $(objs) $(objs-d)

all-source-files=                                \
    $(filter-out $(intermediates),$(wildcard *)) \
    $(wildcard data/*)                           \
    $(wildcard English.lproj/*)                  \
    $(wildcard English.lproj/main.nib/*)         \

$(src-tarball) : $(all-source-files)
	tar --create --gzip --file $@ --transform 's|^|$(basename $(basename $@))/|' --no-recursion --exclude-from .gitignore $^

$(src-zip): $(all-source-files)
	zip -9 $@ $^ --exclude TAGS *.o *~ *.zip

$(bin-zip): $(galaxql-exe) galaxql.dat
	zip -9 $@ $^

clean:
	$(RM) *~ data/*~ $(intermediates) *.exe.stackdump

srctarball: $(src-tarball)

srczip: $(src-zip)

binzip: $(bin-zip)

all: $(galaxql-exe) galaxql.dat

debug: $(galaxql-d-exe)

TAGS: $(galaxql-src) $(sqlite-src) $(wildcard *.h) $(wildcard *.xpm)
	etags $^

.PHONY:binzip srczip srctarball debug clean all
