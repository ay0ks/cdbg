CC = clang
AR = llvm-ar

PREFIX = /usr
INSHDIR = $(PREFIX)/include
INSLDIR = $(PREFIX)/lib
INSL64DIR = $(PREFIX)/lib64

CIFLAGS = -I$(abspath ./include)
CWFLAGS = -Wall -Wshadow -Wextra -Werror
LDLFLAGS = -L$(abspath ./build)

CFLAGS = $(CDFLAGS) $(CIFLAGS) $(CWFLAGS) -std=gnu2y -O2 -fPIC -flto 
LDFLAGS = $(LDLFLAGS)

ifeq ($(clog_DEBUG),1)
  CFLAGS += -g
endif

define add_compile_command
	jq '. += [{ \
		"directory": "$(abspath $(1))", \
		"command": "$(2)", \
		"file": "$(abspath $(3))" \
	}]' $(1)/compile_commands.json > $(1)/compile_commands.json.tmp
	mv $(1)/compile_commands.json.tmp $(1)/compile_commands.json
endef

.DEFAULT:
.PHONY: all
all: build/ build/compile_commands.json build/libclog.a build/libclog.so

.PHONY: install
install: build/ build/libclog.a build/libclog.so include/clog.h
	install -C -m 644 build/libclog.a $(INSLDIR)/
	install -C -m 644 build/libclog.a $(INSL64DIR)/
	install -C -m 755 build/libclog.so $(INSLDIR)/
	install -C -m 755 build/libclog.so $(INSL64DIR)/
	install -C -m 644 include/clog.h $(INSHDIR)/

.PHONY: clean
clean: build/
	rm -rf $^

build/libclog.a: build/ build/clog.c.o
	$(AR) rcs $@ $(word 2,$^)
	$(call add_compile_command,$(@D),$(AR) rcs $@ $(word 2,$^),$(word 2,$^))

build/libclog.so: build/ build/clog.c.o
	$(CC) -o $@ $(word 2,$^) $(CFLAGS) $(LDFLAGS) -shared
	$(call add_compile_command,$(@D),$(CC) -o $@ $(word 2,$^) $(CFLAGS) $(LDFLAGS) -shared,$(word 2,$^))

build/clog.c.o: build/ src/clog.c include/clog.h
	$(CC) -o $@ -c $(word 2,$^) $(CFLAGS)
	$(call add_compile_command,$(@D),$(CC) -o $@ -c $(word 2,$^) $(CFLAGS),$(word 2,$^))

build/:
	mkdir -p $@

build/compile_commands.json: build/
	touch $@
	echo "[]" > $@
