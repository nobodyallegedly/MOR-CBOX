# MOR Agreements verifier (draft 17). C17, warnings as errors.
CC      ?= cc
CLANG   ?= clang
CSTD    := -std=c17
WARN    := -Wall -Wextra -Werror -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wconversion -Wno-sign-conversion -pedantic
CFLAGS  ?= -O2 -g
INC     := -Iinclude -Isrc

# The one cryptographic primitive (SHA-256) comes from OpenSSL libcrypto, pinned to this exact version.
CRYPTO_PIN ?= 3.0.13
CRYPTO_LIB := $(shell pkg-config --libs libcrypto)
CRYPTO_INC := $(shell pkg-config --cflags libcrypto)

SRC := src/set.c src/decode.c src/grammar.c src/legs.c src/stubs.c src/show.c
HDR := include/mor.h src/internal.h
SAN := -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer

all: pin build/libmoragreements.a build/mor-agreements

pin:
	@v=$$(pkg-config --modversion libcrypto); \
	 if [ "$$v" != "$(CRYPTO_PIN)" ]; then echo "libcrypto $$v is not the pinned $(CRYPTO_PIN) (override with CRYPTO_PIN=)"; exit 1; fi

build/%.o: src/%.c $(HDR)
	@mkdir -p build
	$(CC) $(CSTD) $(WARN) $(CFLAGS) $(INC) $(CRYPTO_INC) -c $< -o $@

build/libmoragreements.a: $(SRC:src/%.c=build/%.o)
	ar rcs $@ $^

build/mor-agreements: tools/main.c build/libmoragreements.a
	$(CC) $(CSTD) $(WARN) $(CFLAGS) $(INC) $< build/libmoragreements.a $(CRYPTO_LIB) -o $@

# tests: built and run under AddressSanitizer and UndefinedBehaviorSanitizer
build/test-san: tests/test.c tests/kit.h $(SRC) $(HDR)
	@mkdir -p build
	$(CC) $(CSTD) $(WARN) -O1 -g $(SAN) $(INC) $(CRYPTO_INC) tests/test.c $(SRC) $(CRYPTO_LIB) -o $@

build/mor-agreements-san: tools/main.c $(SRC) $(HDR)
	@mkdir -p build
	$(CC) $(CSTD) $(WARN) -O1 -g $(SAN) $(INC) $(CRYPTO_INC) tools/main.c $(SRC) $(CRYPTO_LIB) -o $@

test: pin build/test-san build/mor-agreements-san
	ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=print_stacktrace=1 build/test-san
	python3 tests/must_coverage.py
	sh tests/cli.sh build/mor-agreements-san

# fuzzing: libFuzzer, with ASan and UBSan
FUZZ := build/fuzz_decode build/fuzz_grammar
build/fuzz_%: fuzz/fuzz_%.c $(SRC) $(HDR) tests/kit.h
	@mkdir -p build
	$(CLANG) $(CSTD) -Wall -Wextra -Werror -O1 -g -fsanitize=fuzzer,address,undefined -fno-sanitize-recover=all $(INC) -Itests $(CRYPTO_INC) $< $(SRC) $(CRYPTO_LIB) -o $@

fuzz-build: $(FUZZ)

FUZZ_SECONDS ?= 60
fuzz: fuzz-build
	build/fuzz_decode  -max_total_time=$(FUZZ_SECONDS) -max_len=8192 -timeout=10 -dict=fuzz/mor.dict -artifact_prefix=fuzz/crashes/decode- fuzz/corpus/decode
	build/fuzz_grammar -max_total_time=$(FUZZ_SECONDS) -max_len=1024 -timeout=10 -artifact_prefix=fuzz/crashes/grammar- fuzz/corpus/grammar

# replay the kept corpus and every kept crash
fuzz-replay: fuzz-build
	build/fuzz_decode  -runs=0 fuzz/corpus/decode
	build/fuzz_grammar -runs=0 fuzz/corpus/grammar
	@for f in fuzz/crashes/*; do case "$$f" in *decode*) build/fuzz_decode "$$f";; *grammar*) build/fuzz_grammar "$$f";; esac || exit 1; done

clean:
	rm -rf build

.PHONY: all pin test fuzz fuzz-build fuzz-replay clean
