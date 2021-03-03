AR = ar
CC = clang
CFLAGS = -D NDEBUG -O3 -Wall -Werror -march=native
CONTROLFILES = control md5sums
DEBPKG = decimal_1.0.0_amd64.deb
HELPER_BINARY = helper
HELPER_SOURCES = helper.c
HELPER_OBJECTS = $(HELPER_SOURCES:%.c=%.o)
LD = clang
LDFLAGS = -Wl,-s
LOG_10_16 = log10_16.txt
RM = rm -fr
TAR = tar
TARFLAGS = --group root --owner root -ch
TARGET_BINARY = usr/local/bin/decimal
TARGET_SOURCES = bcd.c decimal.c
TARGET_OBJECTS = $(TARGET_SOURCES:%.c=%.o)

all: $(DEBPKG)

clean:
	$(RM) $(DEBPKG) $(TARGET_BINARY) $(TARGET_OBJECTS)

control.tar.xz: $(CONTROLFILES)
	$(TAR) $(TARFLAGS) -f - $(CONTROLFILES) | xz -cez9 - > $@ && $(RM) md5sums

data.tar.xz: $(TARGET_BINARY)
	$(TAR) $(TARFLAGS) -f - $(shell for name in `ls`; do test -d $$name && echo $$name; done | xargs) | xz -cez9 - > $@

debian-binary:
	@echo 2.0 > $@

disasm: $(TARGET_BINARY)
	@llvm-objdump --disassemble-all $(TARGET_BINARY) | less

md5sums: $(TARGET_BINARY)
	find . -type f | grep -v -e '/\.' | while read name; do echo -n $$name | cut -d'/' -f2- | grep '/' > /dev/null && echo $$name; done | xargs md5sum > $@

nm: $(TARGET_BINARY)
	@nm -gP --all $^ | less

.PHONY: clean disasm nm

.c.o:
	$(CC) $(CFLAGS) -c $<

$(DEBPKG): control.tar.xz data.tar.xz debian-binary
	$(AR) cr $@ $^ && $(RM) $^

$(HELPER_BINARY): $(HELPER_OBJECTS)
	$(LD) $(LDFLAGS) -lm -o $@ $(HELPER_OBJECTS)
	@$(RM) $(HELPER_OBJECTS)

$(LOG_10_16): $(HELPER_BINARY)
	@./$(HELPER_BINARY) > $@
	@$(RM) $(HELPER_BINARY)

$(TARGET_BINARY): $(TARGET_OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $(TARGET_OBJECTS)
