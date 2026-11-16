STM      := ../swisstm
CFLAGS   += -w -DSTM -DWLPDSTM -I$(STM)/include
CPPFLAGS := $(CFLAGS)
LDFLAGS  += -L$(STM)/lib
LIBS     += -lwlpdstm


.PHONY: default
default: $(PROG)

.PHONY: clean
clean:
	$(RM) $(OBJS) $(PROG) $(OUTPUT)

$(PROG): $(OBJS)
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $(PROG)

include ../common/Makefile.common
