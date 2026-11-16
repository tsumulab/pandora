# ==============================================================================
#
# Defines.common.mk
#
# ==============================================================================


CFLAGS += -DLIST_NO_DUPLICATES
# CFLAGS += -DMAP_USE_RBTREE

CFLAGS += -DMAP_USE_HASHTABLE
CFLAGS += -DHASHTABLE_SIZE=10 # 2^10 (this value is close to Stampede)
CFLAGS += -DUSE_RBTREE_FOR_BUCKET # this also mimics the implementation of Stampede

PROG := vacation

SRCS += \
	client.c \
	customer.c \
	manager.c \
	reservation.c \
	vacation.c \
	$(LIB)/list.c \
	$(LIB)/pair.c \
	$(LIB)/mt19937ar.c \
	$(LIB)/random.c \
	$(LIB)/rbtree.c \
	$(LIB)/thread.c \
	$(LIB)/hashtable.c \
	$(LIB)/memory.c \
#
OBJS := ${SRCS:.c=.o}


# ==============================================================================
#
# End of Defines.common.mk
#
# ==============================================================================
