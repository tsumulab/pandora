# ==============================================================================
#
# Defines.common.mk
#
# ==============================================================================


CFLAGS += -DUSE_EARLY_RELEASE

# We have confirmed that using the queue included with STAMP may be slower 
# depending on the server environment in which it is run. 
# change the queue using cpp library according to your research.
#CFLAGS += -DSTD_QUEUE

# This is a flag that redesigns the Tx definition section, 
# which is difficult to execute transactively due to hardware resource constraints.
# Since fine-lock could only be created in this redesigned version, 
# we have made this flag the default setting for lock comparison and HTM evaluation.
# Please comment out the flag according to your research.
CFLAGS += -DUSE_TMGRID_VALIDATION
# This flag is used to print the number of retries from the grid copy
# when the “USE_TMGRID_VALIDATION” flag is used.
# Use as needed.
# CFLAGS += -DPRINT_RETRY_NUM

# This is a flag that makes a single workqueue that stores the start-end point pairs
# into a workqueue structure for the number of threads.
# However, in labyrinth, this single queue is seldom accessed at the same time,
# so using this flag will cause load imbalance and slowdown.
CFLAGS += -DUSE_DISTRIBUTED_QUEUE
# This flag is used to pop from the workqueue corresponding to another thread ID
# (simple worksteal implementation)
# when the workqueue corresponding to each thread ID becomes empty
# when the “USE_DISTRIBUTED_QUEUE” flag is used.
CFLAGS += -DWORKSTEAL



PROG := labyrinth

SRCS += \
	coordinate.c \
	grid.c \
	labyrinth.c \
	maze.c \
	router.c \
	$(LIB)/list.c \
	$(LIB)/mt19937ar.c \
	$(LIB)/pair.c \
	$(LIB)/queue.c \
	$(LIB)/random.c \
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
