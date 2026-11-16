# ==============================================================================
#
# Makefile.real
#
# ==============================================================================


# ==============================================================================
# Variables
# ==============================================================================

CC      := gcc
CFLAGS  := -g -w -O3
# Disable glibc _FORTIFY_SOURCE: transactions abort via siglongjmp across libfibre
# user-level-thread stacks, which the FORTIFY longjmp check misreads as an
# "uninitialized stack frame" and aborts (intermittently). See tinySTM Makefile.common.
CFLAGS  += -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0
#CFLAGS  += -m32
LD      := gcc

LIBNOREC := libnorec.a

SRCS := \
	norec.c \
#
OBJS := ${SRCS:.c=.o}

AR      := ar
RANLIB  := ranlib

RM := rm -f


# ==============================================================================
# Rules
# ==============================================================================

.PHONY: default
default: lazy

.PHONY: clean
clean:
	$(RM) $(LIBNOREC) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(LIBNOREC): $(OBJS)
	$(AR) cru $@ $^
	$(RANLIB) $@

.PHONY: lazy
lazy: $(LIBNOREC)

.PHONY: otm
otm: CFLAGS += -m32
otm: $(LIBNOREC)


# ==============================================================================
# Dependencies
# ==============================================================================

%.o: %.h


# ==============================================================================
#
# End of Makefile.real
#
# ==============================================================================
