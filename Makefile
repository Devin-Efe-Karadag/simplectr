CC = clang
CPPFLAGS = -D_GNU_SOURCE -Iinclude
CFLAGS = -std=c17 -Wall -Wextra -Wpedantic -Werror -g
ifeq ($(debug),1)
CFLAGS += -O1 -fsanitize=address,undefined -fno-omit-frame-pointer
LDFLAGS += -fsanitize=address,undefined
else
CFLAGS += -O2
endif
LDLIBS = -lmnl -lseccomp -lcap
SOURCES = $(wildcard src/*.c)
OBJECTS = $(patsubst src/%.c,build/%.o,$(SOURCES))
.PHONY: all test privileged-test lint format clean integration-test FORCE
all: bin/simplectr
bin/simplectr: $(OBJECTS)
	@mkdir -p bin
	$(CC) $(LDFLAGS) $(filter %.c %.o,$^) $(LDLIBS) -o $@
build/%.o: src/%.c $(wildcard include/*.h) build/.flags
	@mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
bin/test_cli: tests/test_cli.c src/config.c src/util.c
	@mkdir -p bin
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $(filter %.c %.o,$^) $(LDLIBS) -o $@
bin/test_lifecycle: tests/test_lifecycle.c src/process.c
	@mkdir -p bin
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $(filter %.c %.o,$^) $(LDLIBS) -o $@
test: all bin/test_cli bin/test_lifecycle
	./bin/test_cli
	./bin/test_lifecycle
privileged-test: all bin/test_namespaces bin/test_mounts bin/test_cgroup bin/test_network bin/test_state
	./bin/test_namespaces
	./bin/test_mounts
	./bin/test_cgroup
	./bin/test_network
	./bin/test_state
lint:
	cppcheck --enable=warning,performance,portability --error-exitcode=1 --inline-suppr -Iinclude src tests
format:
	clang-format -i include/*.h src/*.c tests/*.c
clean:
	rm -rf build bin
integration-test: all
	./scripts/test-integration.sh
	./scripts/test-extended.py

bin/test_namespaces: tests/test_namespaces.c src/namespace.c src/process.c
	@mkdir -p bin
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $(filter %.c %.o,$^) $(LDLIBS) -o $@

bin/test_mounts: tests/test_mounts.c src/namespace.c src/process.c src/overlay.c src/mounts.c src/util.c src/security.c
	@mkdir -p bin
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $(filter %.c %.o,$^) $(LDLIBS) -o $@

bin/test_cgroup: tests/test_cgroup.c src/cgroup.c src/config.c src/util.c
	@mkdir -p bin
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $(filter %.c %.o,$^) $(LDLIBS) -o $@

bin/test_network: tests/test_network.c src/netlink.c
	@mkdir -p bin
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $(filter %.c %.o,$^) $(LDLIBS) -o $@

# Rebuild when switching between release and sanitizer flags.
build/.flags: FORCE
	@mkdir -p build
	@echo '$(CPPFLAGS) $(CFLAGS) $(LDFLAGS)' > build/.flags.new
	@cmp -s build/.flags.new $@ || cp build/.flags.new $@
	@rm -f build/.flags.new
bin/test_cli bin/test_lifecycle bin/test_namespaces bin/test_mounts bin/test_cgroup bin/test_network bin/test_state: build/.flags

bin/test_state: tests/test_state.c src/state.c src/config.c src/util.c
	@mkdir -p bin
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $(filter %.c %.o,$^) $(LDLIBS) -o $@
