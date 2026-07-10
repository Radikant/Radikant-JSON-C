============================================================
RADIKANT JSON-C — PERFORMANCE REVIEW
============================================================

SCOPE: decode.c, encode.c, object.c, zone.c, stream.c
NOTE: 1 correctness bug found in rjson_decode_stream (root
      value type gets corrupted). Not a perf item, but fix
      it alongside #1 below since it's the same root cause.
      See prior message for repro if needed.

------------------------------------------------------------
[1] DOUBLE ALLOC + COPY PER NODE  ***PRIMARY BOTTLENECK***
------------------------------------------------------------
Impact   : All Decode benchmarks
Severity : High

Every value (leaf or container) is:
  1. allocated standalone in the zone      (32 bytes/node)
  2. copied via `*el = *value` into the
     parent array/object's contiguous slot (32 byte copy)
  3. original allocation left dead in zone (wasted memory)

    decode_value(..., &element)      // alloc #1
    rjson_array_add(zone, arr, el)   // alloc slot, copy #2

This roughly DOUBLES zone allocation traffic and adds a
32-byte struct copy per node, tree-wide. It also bloats the
zone (worse cache locality on deep trees).

FIX: change decode_* functions to fill a caller-provided
destination in place instead of returning a new pointer:

    rjson_value* slot = rjson_array_append(zone, arr); // reserve first
    decode_value(zone, stream, depth+1, slot);          // fill in place

Expected gain: largest on "Simple" (small nodes -> per-node
overhead dominates). Should also close much of the decode
gap generally.

------------------------------------------------------------
[2] snprintf("%.17g") FALLBACK FOR NON-TRIVIAL FLOATS
------------------------------------------------------------
Impact   : Encoding (Hard) — likely explains the ~9.4x gap
Severity : High

    char buf[64];
    int len = snprintf(buf, sizeof(buf), "%.17g", n);

Integer and 1-decimal fast paths are fine, but any float
that doesn't match those (typical multi-decimal values)
falls to snprintf: locale lookup + format parsing + slow
correct rounding. This is a well-known throughput killer.

FIX: replace with a fast shortest-round-trip algorithm
(Ryu / Grisu / Schubfach). Small, header-only Ryu impls
exist under permissive licenses. This is what yyjson uses
under the hood for its number path.

------------------------------------------------------------
[3] PER-BYTE COPY AFTER FIRST ESCAPE (decode_string)
------------------------------------------------------------
Impact   : Decoding (Diff/Hard) if strings contain escapes
Severity : Medium

Once a string has ANY escape, every subsequent plain
character is still copied one at a time:

    dest[d_idx++] = c;   // per byte, even for long plain runs

FIX: scan ahead to next '\' or '"' and memcpy() the run in
one shot instead of a byte loop.

------------------------------------------------------------
[4] UNDEFINED BEHAVIOR IN isdigit() CALLS (decode_number)
------------------------------------------------------------
Impact   : All Decoding — minor, but also a real bug
Severity : Low-Medium

    if (p >= end || !isdigit(*p)) ...

`*p` is a plain (signed) char; isdigit() requires the arg be
representable as unsigned char or EOF. UB for bytes >= 0x80.
Also a locale-aware call in a hot loop.

FIX: replace with `(unsigned)(c - '0') <= 9` — branch-free,
locale-independent, and typically faster.

------------------------------------------------------------
[5] SMALL INITIAL ENCODE BUFFER
------------------------------------------------------------
Impact   : Encoding, large documents
Severity : Low

    rjson_out_stream_init(&stream, 256);

Doubling growth means this is asymptotically fine, but large
docs still pay several avoidable early realloc+copy cycles.

FIX: size initial buffer off input length / tree size
estimate (e.g. 4-16KB floor, or ~1x source length).

------------------------------------------------------------
SUGGESTED ORDER OF WORK
------------------------------------------------------------
 1. [1] decode in-place refactor      (fixes bug + biggest win)
 2. [2] fast double-to-string         (fixes Encoding Hard)
 3. [3] memcpy runs in unescape
 4. [4] isdigit -> manual digit test
 5. [5] bigger initial encode buffer
============================================================