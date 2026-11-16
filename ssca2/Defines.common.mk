# ==============================================================================
#
# Defines.common.mk
#
# ==============================================================================


PROG := ssca2

SRCS += \
	alg_radix_smp.c \
	computeGraph.c \
	createPartition.c \
	cutClusters.c \
	findSubGraphs.c \
	genScalData.c \
	getStartLists.c \
	getUserParameters.c \
	globals.c \
	ssca2.c \
	$(LIB)/mt19937ar.c \
	$(LIB)/random.c \
	$(LIB)/thread.c \
	$(LIB)/memory.c \
#
OBJS := ${SRCS:.c=.o}

#CFLAGS += -DUSE_PARALLEL_DATA_GENERATION
#CFLAGS += -DWRITE_RESULT_FILES
CFLAGS += -DENABLE_KERNEL1

#This flag is used to change malloc timing and increase parallelism in KERNEL1.
CFLAGS += -DCHANGEMALLOCLOCATION

# This flag is to centralize the Tx definition section in KERNEL1.
CFLAGS += -DCHANGE_TXSIZE

#CFLAGS += -DENABLE_KERNEL2 -DENABLE_KERNEL3
#CFLAGS += -DENABLE_KERNEL4

# This flag is for use with STAMP implementations.
# We have identified a bug in the STAMP structure and are correcting the structure.
# The fixed parts are heap-buffer-overflow and pCutSet must malloc local memoey in KERNEL4.
#CFLAGS += -DORIGNAL### this can't run tcmalloc because of heap-buffer-overflow ###
#CFLAGS += -DORIGINAL_pCutSet ### this is original pCutSet, but this can't output CutSet because pCutSet must malloc local memoey###

# ==============================================================================
#
# End of Defines.common.mk
#
# ==============================================================================
