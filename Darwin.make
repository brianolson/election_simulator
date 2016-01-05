CC = $(CXX)
#CXXFLAGS+=-I/usr/local/BerkeleyDB.4.4/include -DHAVE_OPT_RESET=1
CXXFLAGS+=-DHAVE_OPT_RESET=1
#LDFLAGS+=/usr/local/BerkeleyDB.4.4/lib/libdb.dylib
LDFLAGS+=-L/usr/X11/lib
