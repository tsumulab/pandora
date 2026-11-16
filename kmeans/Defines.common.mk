# ==============================================================================
#
# Defines.common.mk
#
# ==============================================================================


CFLAGS += -DOUTPUT_TO_STDOUT

CFLAGS += -DLENSIZE_64

#　This flag is used to acquire a task from the task range of
# another thread ID (simple workstael implementation)
# when the task range corresponding to its own thread ID is finished.
CFLAGS += -DWORKSTEAL

# This flag allows for larger CHUNK sizes on large data sets,
# which improves performance because it reduces the overheads of TM.
# However, in this benchmark, we would like to find
# room for improvement of TM by properly comparing these issues of TM.
# Therefore, this flag is turned off by default.
# CFLAGS += -DLARGE_CHUNK

# This flag can be used to increase the overhead of the TM; 
# use it as appropriate for TM investigations.
# CFLAGS += -DNO_BATCH

PROG := kmeans

SRCS += \
	cluster.c \
	common.c \
	kmeans.c \
	normal.c \
	$(LIB)/mt19937ar.c \
	$(LIB)/random.c \
	$(LIB)/thread.c \
	$(LIB)/queue.c \
	$(LIB)/memory.c \
#
OBJS := ${SRCS:.c=.o}


# ==============================================================================
#
# End of Defines.common.mk
#
# ==============================================================================
