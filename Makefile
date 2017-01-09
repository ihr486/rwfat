CSRCS := $(wildcard *.c)
COBJS := $(CSRCS:%.c=%.o)
EXE := rwfat
CC := gcc
CFLAGS := -Wall -Wextra -std=c99 -g -Wno-switch
LDFLAGS :=
.PHONY: all clean
all: $(EXE)
clean:
	-@rm -vf $(COBJS) $(EXE)
$(EXE): $(COBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<
