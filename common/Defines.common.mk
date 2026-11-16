# ==============================================================================
#
# Defines.common.mk
#
# ==============================================================================


CC       := gcc
CFLAGS   += -ggdb3 -Wall -pthread
CFLAGS   += -O2
CFLAGS   += -I$(LIB)
CPPFLAGS += -U_FORTIFY_SOURCE
CPPFLAGS += -D_FORTIFY_SOURCE=0
CPP      := g++
CPPFLAGS += $(CFLAGS)
LD       := g++
LIBS     += -lpthread
LIBS     += -lm

LIB := ../lib


# ==============================================================================
#
# End of Defines.common.mk
#
# ==============================================================================
