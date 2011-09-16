# Makefile
# http://www.gnu.org/software/make/manual/make.html

CC = gcc

#flags for compiling the executable
CFLAGS = -Wall -pedantic -g -O3


SOURCE = s3tool.cpp aws_s3.cpp aws_s3_misc.cpp mime_types.cpp

INCLUDEDIRS = -Icurlpp-0.7.3/include/

# defined HAVE_CONFIG_H to make curlpp work
DEFINES = -DHAVE_CONFIG_H

LIBS = -lstdc++ -lssl -lcrypto -lcurl

EXECNAME = s3tool


CSOURCES = $(filter %.c,$(SOURCE))
CPPSOURCES = $(filter %.cpp,$(SOURCE))
MSOURCES = $(filter %.m,$(SOURCE))

OBJECTS = $(CSOURCES:.c=.c.o) \
          $(CPPSOURCES:.cpp=.cpp.o) \
          $(MSOURCES:.m=.m.o)

DEPENDFILES = $(OBJECTS:.o=.d)


CFLAGS := $(CFLAGS) $(DEFINES) $(INCLUDES) $(INCLUDEDIRS)


default: $(EXECNAME) Makefile

$(EXECNAME): curlpp/lib/libcurlpp.a $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) $(LIBS) curlpp/lib/libutilspp.a curlpp/lib/libcurlpp.a -o $(EXECNAME)

curlpp/lib/libcurlpp.a:
	tar -xzf curlpp-0.7.3.tar.gz
	cd curlpp-0.7.3/ ; \
	./configure --prefix=`pwd`/../curlpp/ ; \
	make ; make install


%.c.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.cpp.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

%.m.o: %.m
	$(CC) $(CFLAGS) -c $< -o $@

include $(subst .c,.c.d,$(CSOURCES))
include $(subst .cpp,.cpp.d,$(CPPSOURCES))
include $(subst .m,.m.d,$(MSOURCES))

%.c.d: %.c
	$(CC) -M $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

%.cpp.d: %.cpp
	$(CC) -M $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

%.m.d: %.m
	$(CC) -M $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

install:


clean:
	rm -f $(OBJECTS)
	rm -f $(DEPENDFILES)
	rm -f $(EXECNAME)
