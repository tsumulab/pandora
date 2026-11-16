# ==============================================================================
#
# Defines.common.mk
#
# ==============================================================================


CFLAGS += -DLIST_NO_DUPLICATES
CFLAGS += -DMAP_USE_AVLTREE
# CFLAGS += -DMAP_USE_RBTREE
# CFLAGS += -DMAP_USE_HASHTABLE
# CFLAGS += -DHASHTABLE_SIZE=4
# CFLAGS += -DUSE_RBTREE_FOR_BUCKET
CFLAGS += -DSET_USE_RBTREE

CFLAGS += -DLOCAL_HEAP
CFLAGS += -DNO_SET

PROG := yada
SRCS += \
	coordinate.c \
	element.c \
	mesh.c \
	region.c \
	yada.c \
	$(LIB)/avltree.c \
	$(LIB)/heap.c \
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
