CROSS   ?=
CC      := $(CROSS)gcc
CFLAGS  := -O2 -Wall -Wextra
ifdef SYSROOT
CFLAGS  += -I$(SYSROOT)/usr/include
LDFLAGS += -L$(SYSROOT)/usr/lib/aarch64-linux-gnu -Wl,-rpath-link,$(SYSROOT)/usr/lib/aarch64-linux-gnu
endif

blink: blink.c
	$(CC) $(CFLAGS) $< $(LDFLAGS) -lgpiod -o $@

clean:
	rm -f blink
