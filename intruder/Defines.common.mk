# ==============================================================================
#
# Defines.common.mk
#
# ==============================================================================


# CFLAGS += -DMAP_USE_RBTREE
CFLAGS += -DMAP_USE_HASHTABLE
CFLAGS += -DHASHTABLE_SIZE=16

# This flag is used to distribute the packet queue.
# Contention is eliminated, but performance is degraded due to poor load balance.
# CFLAGS += -DDISTRIDUTEDPACKET

# This flag allows for fine granularity in the transaction definition interval.
# The abort rate is reduced, but the performance is degraded by the overhead.
# CFLAGS += -DFINE_TX

PROG := intruder

SRCS += \
	decoder.c \
	detector.c \
	dictionary.c \
	intruder.c \
	packet.c \
	preprocessor.c \
	stream.c \
	$(LIB)/list.c \
	$(LIB)/mt19937ar.c \
	$(LIB)/pair.c \
	$(LIB)/queue.c \
	$(LIB)/random.c \
	$(LIB)/rbtree.c \
	$(LIB)/thread.c \
	$(LIB)/vector.c \
	$(LIB)/hashtable.c \
	$(LIB)/memory.c \
#
OBJS := ${SRCS:.c=.o}


# ==============================================================================
#
# End of Defines.common.mk
#
# ==============================================================================
