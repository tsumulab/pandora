# ==============================================================================
#
# Defines.common.mk
#
# ==============================================================================


PROG := bayes

SRCS += \
	adtree.c \
	bayes.c \
	data.c \
	learner.c \
	net.c \
	sort.c \
	$(LIB)/bitmap.c \
	$(LIB)/list.c \
	$(LIB)/mt19937ar.c \
	$(LIB)/queue.c \
	$(LIB)/random.c \
	$(LIB)/thread.c \
	$(LIB)/vector.c \
	$(LIB)/memory.c \
#
OBJS := ${SRCS:.c=.o}

CFLAGS += -DLIST_NO_DUPLICATES
CFLAGS += -DLEARNER_TRY_REMOVE
CFLAGS += -DLEARNER_TRY_REVERSE

# In STAMP, the same edge is being deleted and then inserted in 
# the Tx during TMfindBestReverseTask, which causes a lot of conflicts. 
# Commenting out this flag will suppress the conflicts by ignoring the corresponding edge.
# However, we conclude that Bayes is not suitable for evaluating TM performance 
# in the first place. Therefore, for the purpose of performance improvement, 
# we leave this flag at Original as the default.
CFLAGS += -DORIGINAL


# ==============================================================================
#
# End of Defines.common.mk
#
# ==============================================================================
