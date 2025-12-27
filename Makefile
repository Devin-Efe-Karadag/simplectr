	@mkdir -p bin
bin/test_lifecycle: tests/test_lifecycle.c src/process.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $(filter %.c %.o,$^) $(LDLIBS) -o $@
	./bin/test_cli
privileged-test: all bin/test_namespaces bin/test_mounts bin/test_cgroup bin/test_network bin/test_state
	./bin/test_mounts
	./bin/test_state
	cppcheck --enable=warning,performance,portability --error-exitcode=1 --inline-suppr -Iinclude src tests
	clang-format -i include/*.h src/*.c tests/*.c
	rm -rf build bin
	./scripts/test-integration.sh
bin/test_namespaces: tests/test_namespaces.c src/namespace.c src/process.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $(filter %.c %.o,$^) $(LDLIBS) -o $@
	@mkdir -p bin
bin/test_cgroup: tests/test_cgroup.c src/cgroup.c src/config.c src/util.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $(filter %.c %.o,$^) $(LDLIBS) -o $@
	@mkdir -p bin
# Rebuild when switching between release and sanitizer flags.
	@mkdir -p build
	@cmp -s build/.flags.new $@ || cp build/.flags.new $@
bin/test_cli bin/test_lifecycle bin/test_namespaces bin/test_mounts bin/test_cgroup bin/test_network bin/test_state: build/.flags
	@mkdir -p bin
