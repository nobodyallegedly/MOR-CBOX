#!/bin/sh
# Run every fuzz target for SECONDS (default 1800) each, 4 at a time, keeping
# the corpus in fuzz/corpus/<target> and any crash in fuzz/crashes/.
set -u
cd "$(dirname "$0")/.."
secs=${1:-1800}
mkdir -p fuzz/crashes fuzz/logs
run() {
	t=$1
	mkdir -p fuzz/corpus/$t
	./build/fuzz/fuzz_$t -max_total_time="$secs" -max_len=262144 -timeout=30 -rss_limit_mb=4096 \
		-artifact_prefix=fuzz/crashes/$t- -print_final_stats=1 fuzz/corpus/$t > fuzz/logs/$t.log 2>&1
	echo "$t: $(grep -E 'Done|SUMMARY' fuzz/logs/$t.log | tail -1)"
}
for t in cbor text act world; do run $t & done; wait
for t in engine; do run $t & done; wait
for t in people sealed spec json; do run $t & done; wait
