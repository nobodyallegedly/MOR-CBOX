# MOR verifier in C.
#   make            release build: build/rel/morv and build/rel/libmor.a
#   make check      unit tests and every vector, under ASan and UBSan
#   make fuzz       libFuzzer targets in build/fuzz/
#   make deps       fetch and build the pinned libraries (deps/build-deps.sh)
CC      := clang
DEPS    := deps/prefix
WARN    := -Wall -Wextra -Werror -Wpedantic -Wshadow -Wstrict-prototypes -Wmissing-prototypes \
           -Wformat=2 -Wcast-qual -Wpointer-arith -Wvla -Wconversion -Wsign-conversion \
           -Wimplicit-fallthrough -Wundef -Wdouble-promotion -Wnull-dereference -Wcomma \
           -Wunreachable-code -Wswitch-enum -Wmissing-variable-declarations
BASE    := -std=c17 -D_POSIX_C_SOURCE=200809L -I$(DEPS)/include $(WARN)
LIBS    := $(DEPS)/lib/libcrypto.a $(DEPS)/lib/libsodium.a $(DEPS)/lib/libsecp256k1.a \
           $(DEPS)/lib/libutf8proc.a -lpthread -ldl

SRC     := $(wildcard src/*.c)
TOOLSRC := tools/json.c tools/hex.c tools/diag.c tools/vectors.c tools/verdicts.c
REL     := build/rel
SAN     := build/san
FUZ     := build/fuzz

REL_CFLAGS := $(BASE) -O2 -g
SAN_CFLAGS := $(BASE) -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined \
              -fno-sanitize-recover=all -fsanitize=float-divide-by-zero,unsigned-integer-overflow,implicit-conversion,nullability
FUZ_CFLAGS := $(BASE) -O1 -g -fno-omit-frame-pointer -fsanitize=fuzzer-no-link,address,undefined \
              -fno-sanitize-recover=all

all: $(REL)/morv

$(REL)/%.o: %.c $(wildcard src/*.h tools/*.h)
	@mkdir -p $(dir $@)
	$(CC) $(REL_CFLAGS) -c $< -o $@
$(SAN)/%.o: %.c $(wildcard src/*.h tools/*.h)
	@mkdir -p $(dir $@)
	$(CC) $(SAN_CFLAGS) -c $< -o $@
$(FUZ)/%.o: %.c $(wildcard src/*.h tools/*.h)
	@mkdir -p $(dir $@)
	$(CC) $(FUZ_CFLAGS) -c $< -o $@

$(REL)/libmor.a: $(SRC:%.c=$(REL)/%.o)
	ar rcs $@ $^
$(SAN)/libmor.a: $(SRC:%.c=$(SAN)/%.o)
	ar rcs $@ $^
$(FUZ)/libmor.a: $(SRC:%.c=$(FUZ)/%.o)
	ar rcs $@ $^

$(REL)/morv: $(REL)/tools/morv.o $(TOOLSRC:%.c=$(REL)/%.o) $(REL)/libmor.a
	$(CC) $(REL_CFLAGS) $^ $(LIBS) -o $@
$(SAN)/morv: $(SAN)/tools/morv.o $(TOOLSRC:%.c=$(SAN)/%.o) $(SAN)/libmor.a
	$(CC) $(SAN_CFLAGS) $^ $(LIBS) -o $@
$(SAN)/unit: $(SAN)/tests/unit.o $(TOOLSRC:%.c=$(SAN)/%.o) $(SAN)/libmor.a
	$(CC) $(SAN_CFLAGS) $^ $(LIBS) -o $@

FUZZERS := $(patsubst fuzz/%.c,$(FUZ)/%,$(wildcard fuzz/fuzz_*.c))
$(FUZ)/fuzz_%: $(FUZ)/fuzz/fuzz_%.o $(FUZ)/tools/json.o $(FUZ)/libmor.a
	$(CC) $(FUZ_CFLAGS) -fsanitize=fuzzer $^ $(LIBS) -o $@
fuzz: $(FUZZERS)

check: $(SAN)/unit $(SAN)/morv
	$(SAN)/unit
	$(SAN)/morv vectors vectors

deps:
	./deps/build-deps.sh

gcc-check:
	$(MAKE) CC=gcc WARN="-Wall -Wextra -Werror -Wpedantic -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wcast-qual -Wpointer-arith -Wvla -Wconversion -Wsign-conversion -Wimplicit-fallthrough -Wundef -Wdouble-promotion -Wnull-dereference -Wswitch-enum -Wmissing-variable-declarations" REL=build/gcc build/gcc/morv

clean:
	rm -rf build

.PHONY: all check fuzz deps clean gcc-check
