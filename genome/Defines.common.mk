# ==============================================================================
#
# Defines.common.mk
#
# ==============================================================================


CFLAGS += -DLIST_NO_DUPLICATES
CFLAGS += -DCHUNK_STEP1=12


CFLAGS += -DUSE_NORMAL_LIST

# If local hashtables are used in Step1, conflicts are reduced,
# but performance is decreased due to frequent LLC misses.
# Therefore, this flag is turned off by default.
# CFLAGS += -DUSE_LOCAL_HASHTABLE

# Processing Step2a in chunks improves performance because it reduces
# the overheads of TM. However, in this benchmark, we would like to find
# room for improvement of TM by properly comparing these issues of TM.
# Therefore, this flag is turned off by default.
# CFLAGS += -DCHUNK_STEP2A=8


PROG := genome

SRCS += \
	gene.c \
	genome.c \
	segments.c \
	sequencer.c \
	table.c \
	$(LIB)/bitmap.c \
	$(LIB)/hash.c \
	$(LIB)/hashtable.c \
	$(LIB)/pair.c \
	$(LIB)/random.c \
	$(LIB)/list.c \
	$(LIB)/mt19937ar.c \
	$(LIB)/thread.c \
	$(LIB)/vector.c \
	$(LIB)/memory.c \
#
OBJS := ${SRCS:.c=.o}


# ==============================================================================
#
# End of Defines.common.mk
#
# ==============================================================================
