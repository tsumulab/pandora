CFLAGS += -DOUTPUT_TO_STDOUT
CFLAGS += -DLENSIZE_64

PROG := kmeans

SRCS += \
	cluster.c \
	common.c \
	kmeans.c \
	normal.c \
	$(LIB)/mt19937ar.c \
	$(LIB)/random.c \
	$(LIB)/thread.c \
	$(LIB)/memory.c \
#
OBJS := ${SRCS:.c=.o}
