# Agreements verifier (draft 19). C11, warnings as errors.
# The one cryptographic primitive, SHA-256, comes from OpenSSL libcrypto,
# pinned to the 3.x series (built and tested with 3.0.13); see REPORT.md.
CC      ?= cc
CSTD    = -std=c11
WARN    = -Wall -Wextra -Werror -Wpedantic -Wshadow -Wconversion -Wstrict-prototypes -Wmissing-prototypes
CFLAGS  ?= -O2 -g
CPPFLAGS = -Iinclude -D_POSIX_C_SOURCE=200809L
LDLIBS  = -lcrypto
SAN     = -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer
SRC     = src/core.c src/standin.c src/decode.c src/draft.c src/stubs.c src/engine.c src/report.c
HDR     = include/mor.h src/internal.h
B       = build

all: $(B)/libmor.a $(B)/morv

$(B)/obj/%.o: src/%.c $(HDR)
	@mkdir -p $(B)/obj
	$(CC) $(CSTD) $(WARN) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(B)/libmor.a: $(SRC:src/%.c=$(B)/obj/%.o)
	$(AR) rcs $@ $^

$(B)/morv: tools/morv.c $(B)/libmor.a
	$(CC) $(CSTD) $(WARN) $(CFLAGS) $(CPPFLAGS) $< $(B)/libmor.a $(LDLIBS) -o $@

# tests: built and run under AddressSanitizer and UndefinedBehaviorSanitizer
$(B)/test_mor: tests/test_mor.c $(SRC) $(HDR)
	@mkdir -p $(B)
	$(CC) $(CSTD) $(WARN) -O1 -g $(SAN) $(CPPFLAGS) tests/test_mor.c $(SRC) $(LDLIBS) -o $@

$(B)/morv_san: tools/morv.c $(SRC) $(HDR)
	@mkdir -p $(B)
	$(CC) $(CSTD) $(WARN) -O1 -g $(SAN) $(CPPFLAGS) tools/morv.c $(SRC) $(LDLIBS) -o $@

test: $(B)/test_mor $(B)/morv_san
	ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=print_stacktrace=1 $(B)/test_mor
	sh tests/cli.sh $(B)/morv_san

# the same tests with the other compiler, also under the sanitizers
test-gcc:
	$(MAKE) CC=gcc B=build-gcc test
test-clang:
	$(MAKE) CC=clang B=build-clang test

# fuzzing: libFuzzer (clang), with ASan and UBSan
FUZZ_CC = clang
FUZZ_FLAGS = $(CSTD) $(WARN) -O1 -g -fsanitize=fuzzer,address,undefined -fno-sanitize-recover=all $(CPPFLAGS)
FUZZERS = $(B)/fuzz_decode $(B)/fuzz_draft $(B)/fuzz_engine
$(B)/fuzz_%: fuzz/fuzz_%.c $(SRC) $(HDR)
	@mkdir -p $(B)
	$(FUZZ_CC) $(FUZZ_FLAGS) $< $(SRC) $(LDLIBS) -o $@
fuzzers: $(FUZZERS)
FUZZ_SECONDS ?= 60
# the corpus the fuzzers arrived at is kept, minimised, in fuzz/corpus.tar.gz
fuzz/corpus/stamp: fuzz/corpus.tar.gz
	tar -C fuzz -xzf fuzz/corpus.tar.gz
	touch $@
# seeds: the sealed acts and the drafts of every test case, the example, and a few engine programs
$(B)/seeds/stamp: $(B)/test_mor
	mkdir -p $(B)/seeds/tmp $(B)/seeds/decode $(B)/seeds/draft $(B)/seeds/engine
	MOR_DUMP_DIR=$(B)/seeds/tmp $(B)/test_mor > /dev/null
	for f in $(B)/seeds/tmp/*.acts; do { printf '\001'; cat $$f; } > $(B)/seeds/decode/$$(basename $$f); done
	cp $(B)/seeds/tmp/*.draft examples/*.draft $(B)/seeds/draft/
	cp fuzz/seeds/engine/* $(B)/seeds/engine/
	touch $@
fuzz: fuzzers $(B)/seeds/stamp fuzz/corpus/stamp
	for f in decode draft engine; do \
	  mkdir -p fuzz/corpus/$$f fuzz/crashes/$$f; \
	  $(B)/fuzz_$$f -max_total_time=$(FUZZ_SECONDS) -timeout=20 -rss_limit_mb=2048 \
	     -artifact_prefix=fuzz/crashes/$$f/ fuzz/corpus/$$f $(B)/seeds/$$f || exit 1; \
	done

# every input a fuzzer ever stopped on is kept in fuzz/crashes and must now run clean
fuzz-regress: fuzzers
	for f in decode draft engine; do \
	  for x in fuzz/crashes/$$f/*; do [ -f "$$x" ] || continue; $(B)/fuzz_$$f -timeout=20 "$$x" > /dev/null 2>&1 || { echo "FAIL $$x"; exit 1; }; echo "ok   $$x"; done; \
	done

clean:
	rm -rf build build-gcc build-clang

.PHONY: all test test-gcc test-clang fuzzers fuzz fuzz-regress clean
