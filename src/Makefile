FLAGS += -pthread
CFLAGS += -g -pedantic -Werror -Wall -std=c99 -D_REENTRANT -D_GNU_SOURCE -D_POSIX_SOURCE -D_POSIX_C_SOURCE=200112L
LIBS += -lm -lsml -luuid -lcurl
OBJS = \
	pylon/meter.o \
	pylon/smartmeter.o \
	pylon/fluksometer.o \
	pylon/io.o \
	pylon/ip.o \
	pylon/uploader.o \
	pylon/queue.o \
	pylon/strbuilder.o \
	pylon/timer.o \
	pylon/args.o \
	pylon/common.o

all : smlogger

smlogger : smlogger.o $(OBJS)
	$(CC) $(FLAGS) $(LDFLAGS) smlogger.o $(OBJS) $(LIBS) -o smlogger

%.o : %.c
	$(CC) $(FLAGS) $(CFLAGS) -c $^ -o $@

clean:
	@rm -f pylon/*.o
	@rm -f *.o
	@rm -f smlogger

