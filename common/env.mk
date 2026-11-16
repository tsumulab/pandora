WORK_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))/..

################################################################################
# Select memory allocator
#ALLOCATOR := tlmalloc
 ALLOCATOR := tcmalloc

# Use user-level-thread (ULT) : libfibre
USE_ULT := libfibre

# Enable thread switching (abort-triggered fibre yield; paper Section 5.1).
#   Default: on. Build the "non-switch" variant with  NO_SWITCH=1  in the
#   environment (used by ae/build_all.sh); no need to edit this file.
ifneq ($(NO_SWITCH),1)
CFLAGS += -DTHREAD_SWITCHING
endif
thr_per_core ?= 4

# Extra flags injected by the AE scripts, e.g. AE_CFLAGS=-DTINYSTM_STATS
# for the instrumented TinySTM build used for Table 4.
CFLAGS += $(AE_CFLAGS)

# Enable Core-bind
#   Please set the bind-policy yourself
#   according to your own environment (in lib/thread.c).
# CFLAGS += -DENABLE_BIND
################################################################################


################################################################################
# use tlmalloc
ifeq ($(ALLOCATOR), tlmalloc)
	CFLAGS += -DUSE_TLMALLOC
endif

# use tcmalloc
ifeq ($(ALLOCATOR), tcmalloc)
	LIBS += -ltcmalloc
	CFLAGS += -DUSE_TCMALLOC
	# the paper's TCMalloc (allocator/README.md), built by ae/build_allocator.sh
	LDFLAGS += -L$(WORK_DIR)/allocator/lib -Wl,-rpath,$(WORK_DIR)/allocator/lib '-Wl,-rpath,$$ORIGIN/../../allocator/lib'
endif
################################################################################


################################################################################
ULT_DIR := $(WORK_DIR)/ULT

# use libfibre
ifeq ($(USE_ULT), libfibre)
	FIBRE_DIR := $(ULT_DIR)/libfibre/src
	WRAP_DIR := $(ULT_DIR)/fibre
	ifdef thr_per_core
		CFLAGS += -DTHREADS_PER_CORE=$(thr_per_core)
	else
		CFLAGS += -DTHREADS_PER_CORE=1
	endif
	CFLAGS += -DLIBFIBRE @$(WRAP_DIR)/libfibre.opts -I$(FIBRE_DIR)
	LDFLAGS += @$(WRAP_DIR)/libfibre.opts -L$(FIBRE_DIR) -Wl,-R$(FIBRE_DIR) '-Wl,-R,$$ORIGIN/../../ULT/libfibre/src'
	LIBS += -lfibre
	SRCS += $(WRAP_DIR)/libfibre_wrap_pthread.c
	OBJS += $(WRAP_DIR)/libfibre_wrap_pthread.o
endif
################################################################################
