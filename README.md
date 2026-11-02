# CS 425 Project 3: CIDR Routing

- Name: Seth McDaniel
- Email: sethmcdaniel547@u.boisestate.edu
- Class: 425

This project implements the longest-prefix lookup a router performs on every
packet: given a destination address and a forwarding table, it finds the most
specific prefix that matches. Along the way it implements the IPv4 address and
CIDR arithmetic that dotted-quad and slash notation hide.

## Building and testing

The project uses GNU Make, GCC or Clang, Unity, AddressSanitizer, and gcovr:

```bash
make all         # build debug, release, test, and debug-test
make check       # run the unit tests
make report      # run tests and produce the coverage report
make leak        # run the release program under ASan (no-argument path)
make leak-test   # run the unit tests under ASan
```

The release executable is `build/release/myapp`.

```text
myapp info <address>/<prefix>
myapp split <network>/<prefix> <count>
myapp lookup <table-file> <address>...
```

### Examples

```text
$ ./build/release/myapp info 132.178.227.11/25
address    132.178.227.11
prefix     /25
netmask    255.255.255.128
network    132.178.227.0
broadcast  132.178.227.127
first host 132.178.227.1
last host  132.178.227.126
hosts      126

$ ./build/release/myapp split 192.168.1.0/24 4
192.168.1.0/26
192.168.1.64/26
192.168.1.128/26
192.168.1.192/26

$ ./build/release/myapp lookup tables/example.txt 132.178.227.50 8.8.8.8 10.1.2.5 10.1.2.200
132.178.227.50 dev eno1
8.8.8.8 via 132.178.227.1 dev eno1
10.1.2.5 dev eth2
10.1.2.200 via 10.1.2.1 dev eth2
```

The program exits `0` on success, `1` for a bad command line or a malformed
address, `2` when the table file cannot be opened or contains a bad line, and
`3` when an address matches no route. When a single lookup run mixes errors, it
reports every address and exits `1` if any address was malformed, otherwise `3`.

## Design

IPv4 addresses are stored as a `uint32_t` in host byte order, so the first
octet of `192.168.1.10` is the most significant byte (`0xC0A8010A`). Parsing is
deliberately strict and hand-written rather than using `inet_aton()` or
`sscanf()`: each octet is decimal, in range 0–255, with no leading zero, and the
whole string must be consumed, so `010.0.0.1`, `256.1.1.1`, `1.2.3`, and
`1.2.3.4x` are all rejected. A router cannot afford to guess at malformed input
because a guess becomes a route to the wrong place.

### The `/0` default route

The subnet mask is `0xFFFFFFFF << (32 - prefix)`, but that shift is undefined
behavior in C when the prefix is 0 (a 32-bit shift by 32). On x86 the processor
would shift by 0 and hand back `0xFFFFFFFF`, which makes the default route match
only `0.0.0.0`. `cidr_mask()` therefore returns `0` explicitly for a `/0`, so
the mask is all zeros, the network is `0.0.0.0`, and the block contains every
address. The default route is represented as the ordinary block `0.0.0.0/0`; it
is not a special case in the lookup, it simply has the shortest possible prefix
and so loses to anything more specific.

### The `/31` and `/32` boundary prefixes

A `/31` is a point-to-point link (RFC 3021) where both addresses are usable
hosts, and a `/32` is a single host. Both break the usual
network/broadcast/usable-range pattern, so `cidr_first_host()`,
`cidr_last_host()`, and `cidr_host_count()` handle them before applying the
general formula: a `/32` has one host (first == last == the address) and a `/31`
has two (first == network, last == broadcast). The host-count formula for a
`/0` is `2^32 - 2`, and because `2^32` overflows a `uint32_t`, `cidr_host_count()`
computes it in 64 bits and returns a `uint64_t`.

### Longest-prefix match

The forwarding table owns a growable array of routes (grown with `realloc()`,
freed in full by `table_destroy()`). `table_add()` refuses a second route to the
same block, the way `ip route add` does. `table_lookup()` scans the whole table
and keeps the matching route with the greatest prefix length, so its answer does
not depend on the order routes were added — a lookup that returned the first
match would pass only tables that happen to be sorted longest-first. A linear
scan is adequate here; a real router would walk a binary trie instead.

## Known Bugs or Issues

None known. All unit tests pass, `make report` shows 100% line coverage of
`src/lab.c`, and `make leak` and `make leak-test` report no leaks or crashes
under AddressSanitizer.

## Experience

The subtle parts were the undefined `/0` shift and the `/31` and `/32` host
rules, which is exactly where the boundary tests paid off. Writing a strict
parser by hand — rejecting leading zeros, trailing junk, and out-of-range octets
— was more involved than reaching for a library function, but it is the correct
behavior for routing input.
