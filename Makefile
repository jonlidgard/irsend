program_NAME := irsend
program_C_SRCS := $(wildcard *.c)
program_CXX_SRCS := $(wildcard *.cpp)
program_ASM_SRCS := $(wildcard *.p)
program_PRU_EXT := _pru
program_PRU := $(program_NAME)$(program_PRU_EXT)
program_PRU_SRC := $(program_PRU).p
program_PRU_BIN := $(program_PRU)_bin.h
program_C_OBJS := ${program_C_SRCS:.c=.o}
program_CXX_OBJS := ${program_CXX_SRCS:.cpp=.o}
program_OBJS := $(program_C_OBJS) $(program_CXX_OBJS)
program_INCLUDE_DIRS :=
program_LIBRARY_DIRS :=
program_LIBRARIES := config prussdrv

PASM := pasm -c -CprussPru0Code
CPPFLAGS += $(foreach includedir,$(program_INCLUDE_DIRS),-I$(includedir))
LDFLAGS += $(foreach librarydir,$(program_LIBRARY_DIRS),-L$(librarydir))
LDFLAGS += $(foreach library,$(program_LIBRARIES),-l$(library))

.PHONY: all clean distclean

all: $(program_NAME)

$(program_PRU_BIN): Makefile $(program_PRU_SRC)
		 $(PASM) $(program_PRU_SRC) $(program_PRU)

$(program_NAME): $(program_PRU_BIN) $(program_OBJS) 
		$(LINK.cc) $(program_OBJS) -o $(program_NAME)

clean:
		@- $(RM) $(program_NAME)
		@- $(RM) $(program_OBJS)
		@- $(RM) $(program_PRU_BIN)

distclean: clean


#.PHONY: clean all
#all: irsend
#clean:
#	rm -f irsendpru_bin.h irsend
#irsend: Makefile irsend.c irsendpru_bin.h
#irsend.o:
#libpru.o: libpru.c libpru.h
#  gcc -o libpru.c -I./
#	gcc -o irsend -g -l prussdrv -l config irsend.c -I./ libpru.c
