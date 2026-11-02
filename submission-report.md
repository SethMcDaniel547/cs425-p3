# Submission Report

- Submission generated at 11/02/2026 at 01:41:45

- Machine info: Linux runnervmnwato 7.0.0-1017-azure #17-Ubuntu SMP PREEMPT Thu Sep 24 15:54:51 UTC 2026 x86_64 GNU/Linux

## Note to Students

Please read this report carefully before submission.
Ensure that all sections are complete and accurate.
Look for any errors in the build or test outputs.
If you find any issues, correct them before submitting.
Post any questions on the class discussion board for help.


---

## README

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

---


## Build Output

This section was generated by running `make all` in the project root directory.

```bash
make[1]: Entering directory '/home/runner/work/cs425-p3/cs425-p3'
mkdir -p build/debug
cc -g -O0 -DDEBUG -fno-omit-frame-pointer -fsanitize=address -c src/main.c -o build/debug/main.c.o
mkdir -p build/debug
cc -g -O0 -DDEBUG -fno-omit-frame-pointer -fsanitize=address -c src/lab.c -o build/debug/lab.c.o
cc -g -O0 -DDEBUG -fno-omit-frame-pointer -fsanitize=address build/debug/main.c.o build/debug/lab.c.o -o build/debug/myapp_d -fsanitize=address
make[1]: Leaving directory '/home/runner/work/cs425-p3/cs425-p3'
make[1]: Entering directory '/home/runner/work/cs425-p3/cs425-p3'
mkdir -p build/release
cc -Wall -Wextra -O2 -fPIE -MMD -MP -Wformat -Wformat=2 -Wconversion -Wsign-conversion -Wimplicit-fallthrough -fstack-protector-strong -Werror=format-security -Werror=implicit -Werror=incompatible-pointer-types -Werror=int-conversion -c src/main.c -o build/release/main.c.o
mkdir -p build/release
cc -Wall -Wextra -O2 -fPIE -MMD -MP -Wformat -Wformat=2 -Wconversion -Wsign-conversion -Wimplicit-fallthrough -fstack-protector-strong -Werror=format-security -Werror=implicit -Werror=incompatible-pointer-types -Werror=int-conversion -c src/lab.c -o build/release/lab.c.o
cc -Wall -Wextra -O2 -fPIE -MMD -MP -Wformat -Wformat=2 -Wconversion -Wsign-conversion -Wimplicit-fallthrough -fstack-protector-strong -Werror=format-security -Werror=implicit -Werror=incompatible-pointer-types -Werror=int-conversion build/release/main.c.o build/release/lab.c.o -o build/release/myapp 
make[1]: Leaving directory '/home/runner/work/cs425-p3/cs425-p3'
make[1]: Entering directory '/home/runner/work/cs425-p3/cs425-p3'
mkdir -p build/tests
cc -g -O0 -DTEST -fprofile-arcs -ftest-coverage -c src/main.c -o build/tests/main.c.o
mkdir -p build/tests
cc -g -O0 -DTEST -fprofile-arcs -ftest-coverage -c src/lab.c -o build/tests/lab.c.o
mkdir -p build/tests/harness/
cc -g -O0 -DTEST -fprofile-arcs -ftest-coverage -c tests/harness/unity.c -o build/tests/harness/unity.c.o
mkdir -p build/tests/
cc -g -O0 -DTEST -fprofile-arcs -ftest-coverage -c tests/lab-test.c -o build/tests/lab-test.c.o
cc -g -O0 -DTEST -fprofile-arcs -ftest-coverage build/tests/main.c.o build/tests/lab.c.o build/tests/harness/unity.c.o build/tests/lab-test.c.o -o build/tests/myapp_t -fprofile-arcs -ftest-coverage
make[1]: Leaving directory '/home/runner/work/cs425-p3/cs425-p3'
make[1]: Entering directory '/home/runner/work/cs425-p3/cs425-p3'
mkdir -p build/debug-test
cc -g -O0 -DDEBUG -DTEST -fno-omit-frame-pointer -fsanitize=address -c src/main.c -o build/debug-test/main.c.o
mkdir -p build/debug-test
cc -g -O0 -DDEBUG -DTEST -fno-omit-frame-pointer -fsanitize=address -c src/lab.c -o build/debug-test/lab.c.o
mkdir -p build/debug-test/harness/
cc -g -O0 -DDEBUG -DTEST -fno-omit-frame-pointer -fsanitize=address -c tests/harness/unity.c -o build/debug-test/harness/unity.c.o
mkdir -p build/debug-test/
cc -g -O0 -DDEBUG -DTEST -fno-omit-frame-pointer -fsanitize=address -c tests/lab-test.c -o build/debug-test/lab-test.c.o
cc -g -O0 -DDEBUG -DTEST -fno-omit-frame-pointer -fsanitize=address build/debug-test/main.c.o build/debug-test/lab.c.o build/debug-test/harness/unity.c.o build/debug-test/lab-test.c.o -o build/debug-test/myapp_td -fsanitize=address
make[1]: Leaving directory '/home/runner/work/cs425-p3/cs425-p3'
Builds completed. You can run the application with: ./build/release/myapp
You can run the debug build with: ./build/debug/myapp_d
You can run the test build with: ./build/tests/myapp_t
You can run the debug-test build with: ./build/debug-test/myapp_td
```

---

## Coverage Report

This section was generated by running `make report` in the project root directory.

```bash
tests/lab-test.c:260:test_ip_parse_and_format:PASS
tests/lab-test.c:261:test_cidr_parse_and_format:PASS
tests/lab-test.c:262:test_cidr_arithmetic_boundaries:PASS
tests/lab-test.c:263:test_cidr_subnet:PASS
tests/lab-test.c:264:test_routes_and_table:PASS
tests/lab-test.c:265:test_longest_prefix_independent_of_order:PASS
tests/lab-test.c:266:test_table_growth_and_no_default:PASS
tests/lab-test.c:267:test_table_null_safety:PASS
tests/lab-test.c:268:test_table_add_rejects_host_bits:PASS
tests/lab-test.c:269:test_table_load_bad_line:PASS
tests/lab-test.c:270:test_table_load_success:PASS

-----------------------
11 Tests 0 Failures 0 Ignored 
OK
./build/tests/myapp_t
tests/lab-test.c:260:test_ip_parse_and_format:PASS
tests/lab-test.c:261:test_cidr_parse_and_format:PASS
tests/lab-test.c:262:test_cidr_arithmetic_boundaries:PASS
tests/lab-test.c:263:test_cidr_subnet:PASS
tests/lab-test.c:264:test_routes_and_table:PASS
tests/lab-test.c:265:test_longest_prefix_independent_of_order:PASS
tests/lab-test.c:266:test_table_growth_and_no_default:PASS
tests/lab-test.c:267:test_table_null_safety:PASS
tests/lab-test.c:268:test_table_add_rejects_host_bits:PASS
tests/lab-test.c:269:test_table_load_bad_line:PASS
tests/lab-test.c:270:test_table_load_success:PASS

-----------------------
11 Tests 0 Failures 0 Ignored 
OK
mkdir -p ./build/report/html
mkdir -p ./build/report/txt
gcovr -r . --html --html-details --exclude-directories build/tests/harness --exclude '.*main\.c$' --exclude '.*test\.c$' -o ./build/report/html/coverage_report.html
gcovr -r . --txt                 --exclude-directories build/tests/harness --exclude '.*main\.c$' --exclude '.*test\.c$'
------------------------------------------------------------------------------
                           GCC Code Coverage Report
Directory: .
------------------------------------------------------------------------------
File                                         Lines     Exec  Cover   Missing
------------------------------------------------------------------------------
src/lab.c                                      158      158   100%
------------------------------------------------------------------------------
TOTAL                                          158      158   100%
------------------------------------------------------------------------------
```

---

## Address Sanitizer Report

This section was generated by running `make leak-test` in the project root directory.

```bash
tests/lab-test.c:260:test_ip_parse_and_format:PASS
tests/lab-test.c:261:test_cidr_parse_and_format:PASS
tests/lab-test.c:262:test_cidr_arithmetic_boundaries:PASS
tests/lab-test.c:263:test_cidr_subnet:PASS
tests/lab-test.c:264:test_routes_and_table:PASS
tests/lab-test.c:265:test_longest_prefix_independent_of_order:PASS
tests/lab-test.c:266:test_table_growth_and_no_default:PASS
tests/lab-test.c:267:test_table_null_safety:PASS
tests/lab-test.c:268:test_table_add_rejects_host_bits:PASS
tests/lab-test.c:269:test_table_load_bad_line:PASS
tests/lab-test.c:270:test_table_load_success:PASS

-----------------------
11 Tests 0 Failures 0 Ignored 
OK
```

---

## Src Files
### lab.c

```c

#include "lab.h"

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

struct fwd_table {
  route_t *routes;
  size_t count;
  size_t capacity;
};

bool ip_parse(const char *text, uint32_t *out)
{
  if (text == NULL || out == NULL) return false;
  uint32_t value = 0;
  const char *cursor = text;
  for (unsigned octet = 0; octet < 4; octet++) {
    if (!isdigit((unsigned char)*cursor)) return false;
    unsigned number = 0;
    unsigned digits = 0;
    while (isdigit((unsigned char)*cursor)) {
      if (digits++ == 0 && *cursor == '0' && isdigit((unsigned char)cursor[1])) return false;
      number = number * 10U + (unsigned)(*cursor - '0');
      if (number > 255U) return false;
      cursor++;
    }
    value = (value << 8) | number;
    if (octet < 3) {
      if (*cursor != '.') return false;
      cursor++;
    }
  }
  if (*cursor != '\0') return false;
  *out = value;
  return true;
}

void ip_format(uint32_t address, char out[IP_STR_LEN])
{
  (void)snprintf(out, IP_STR_LEN, "%u.%u.%u.%u",
                 (unsigned)((address >> 24) & 255U),
                 (unsigned)((address >> 16) & 255U),
                 (unsigned)((address >> 8) & 255U),
                 (unsigned)(address & 255U));
}

bool cidr_parse(const char *text, cidr_t *out)
{
  if (text == NULL || out == NULL) return false;
  const char *slash = strchr(text, '/');
  if (slash == NULL || slash == text || strchr(slash + 1, '/') != NULL) return false;
  size_t address_length = (size_t)(slash - text);
  if (address_length >= 16) return false;
  char address_text[16];
  memcpy(address_text, text, address_length);
  address_text[address_length] = '\0';
  uint32_t address;
  if (!ip_parse(address_text, &address)) return false;
  const char *prefix_text = slash + 1;
  if (*prefix_text == '\0' || (*prefix_text == '0' && prefix_text[1] != '\0')) return false;
  unsigned prefix = 0;
  unsigned digits = 0;
  while (*prefix_text != '\0') {
    if (!isdigit((unsigned char)*prefix_text)) return false;
    prefix = prefix * 10U + (unsigned)(*prefix_text - '0');
    if (++digits > 2 || prefix > 32U) return false;
    prefix_text++;
  }
  out->addr = address;
  out->prefix = (uint8_t)prefix;
  return true;
}

void cidr_format(cidr_t block, char out[19])
{
  char address[16];
  ip_format(block.addr, address);
  size_t length = strlen(address);
  memcpy(out, address, length);
  out[length++] = '/';
  uint8_t prefix = block.prefix > 32 ? 32 : block.prefix;
  if (prefix >= 10) out[length++] = (char)('0' + prefix / 10);
  out[length++] = (char)('0' + prefix % 10);
  out[length] = '\0';
}

uint32_t cidr_mask(uint8_t prefix)
{
  if (prefix == 0) return 0;
  if (prefix >= 32) return UINT32_MAX;
  return UINT32_MAX << (32U - prefix);
}

uint32_t cidr_network(cidr_t block) { return block.addr & cidr_mask(block.prefix); }
uint32_t cidr_broadcast(cidr_t block) { return cidr_network(block) | ~cidr_mask(block.prefix); }

uint32_t cidr_first_host(cidr_t block)
{
  if (block.prefix >= 31) return cidr_network(block);
  return cidr_network(block) + 1U;
}

uint32_t cidr_last_host(cidr_t block)
{
  if (block.prefix == 32) return cidr_network(block);
  if (block.prefix == 31) return cidr_broadcast(block);
  return cidr_broadcast(block) - 1U;
}

uint64_t cidr_host_count(cidr_t block)
{
  if (block.prefix == 32) return 1;
  if (block.prefix == 31) return 2;
  uint64_t count = UINT64_C(1) << (32U - block.prefix);
  return count - 2U;
}

bool cidr_contains(cidr_t block, uint32_t address)
{
  uint32_t mask = cidr_mask(block.prefix);
  return (address & mask) == (block.addr & mask);
}

bool cidr_is_network(cidr_t block) { return block.addr == cidr_network(block); }

bool cidr_subnet(cidr_t block, uint32_t count, uint32_t index, cidr_t *out)
{
  if (out == NULL || count == 0 || (count & (count - 1U)) != 0 ||
      block.prefix > 32 || index >= count || !cidr_is_network(block)) return false;
  unsigned bits = 0;
  for (uint32_t n = count; n > 1; n >>= 1) bits++;
  if ((unsigned)block.prefix + bits > 32U) return false;
  out->prefix = (uint8_t)(block.prefix + bits);
  uint64_t size = UINT64_C(1) << (32U - out->prefix);
  out->addr = block.addr + (uint32_t)((uint64_t)index * size);
  return true;
}

bool route_parse(const char *text, route_t *out)
{
  if (text == NULL || out == NULL) return false;
  char buffer[256];
  size_t length = strlen(text);
  if (length >= sizeof(buffer)) return false;
  memcpy(buffer, text, length + 1);
  char destination[32], gateway[32], device[DEV_NAME_LEN];
  char extra;
  int fields = sscanf(buffer, " %31s %31s %15s %c", destination, gateway, device, &extra);
  if (fields != 3 || device[0] == '\0') return false;
  cidr_t block;
  if (strcmp(destination, "default") == 0) {
    block.addr = 0;
    block.prefix = 0;
  } else if (!cidr_parse(destination, &block)) {
    return false;
  }
  if (!cidr_is_network(block)) return false;
  out->dest = block;
  if (strcmp(gateway, "direct") == 0) {
    out->gateway = 0;
  } else if (!ip_parse(gateway, &out->gateway)) {
    return false;
  }
  if (strlen(device) >= DEV_NAME_LEN) return false;
  (void)snprintf(out->dev, sizeof(out->dev), "%s", device);
  return true;
}

fwd_table_t *table_create(void) { return calloc(1, sizeof(fwd_table_t)); }

void table_destroy(fwd_table_t *table)
{
  if (table == NULL) return;
  free(table->routes);
  free(table);
}

size_t table_count(const fwd_table_t *table) { return table == NULL ? 0 : table->count; }

bool table_add(fwd_table_t *table, const route_t *route)
{
  if (table == NULL || route == NULL || !cidr_is_network(route->dest)) return false;
  for (size_t i = 0; i < table->count; i++) {
    if (table->routes[i].dest.addr == route->dest.addr &&
        table->routes[i].dest.prefix == route->dest.prefix) return false;
  }
  if (table->count == table->capacity) {
    size_t capacity = table->capacity == 0 ? 8 : table->capacity * 2;
    route_t *routes = realloc(table->routes, capacity * sizeof(*routes));
    if (routes == NULL) return false;
    table->routes = routes;
    table->capacity = capacity;
  }
  table->routes[table->count++] = *route;
  return true;
}

bool table_load(fwd_table_t *table, FILE *file, size_t *bad_line)
{
  if (bad_line != NULL) *bad_line = 0;
  if (table == NULL || file == NULL) return false;
  char *line = NULL;
  size_t capacity = 0;
  size_t line_number = 0;
  bool success = true;
  while (getline(&line, &capacity, file) >= 0) {
    line_number++;
    char *cursor = line;
    while (isspace((unsigned char)*cursor)) cursor++;
    if (*cursor == '\0' || *cursor == '#') continue;
    char *comment = strchr(line, '#');
    if (comment != NULL) *comment = '\0';
    route_t route;
    if (!route_parse(line, &route) || !table_add(table, &route)) {
      if (bad_line != NULL) *bad_line = line_number;
      success = false;
      break;
    }
  }
  free(line);
  if (ferror(file)) return false;
  return success;
}

const route_t *table_lookup(const fwd_table_t *table, uint32_t address)
{
  if (table == NULL) return NULL;
  const route_t *best = NULL;
  for (size_t i = 0; i < table->count; i++) {
    if (cidr_contains(table->routes[i].dest, address) &&
        (best == NULL || table->routes[i].dest.prefix > best->dest.prefix)) {
      best = &table->routes[i];
    }
  }
  return best;
}

```

### lab.h

```c

#ifndef LAB_H
#define LAB_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/*
 * Every address in this file is a uint32_t in host byte order, so the first
 * octet of 192.168.1.10 is the most significant byte: 0xC0A8010A.
 */

/** Room for the longest dotted quad, "255.255.255.255", and its '\0'. */
#define IP_STR_LEN 16

/** Room for the longest CIDR string, "255.255.255.255/32", and its '\0'. */
#define CIDR_STR_LEN 19

/** Room for an interface name; IFNAMSIZ on Linux, including the '\0'. */
#define DEV_NAME_LEN 16

/** An address block in CIDR notation: an address and a prefix length. */
typedef struct {
  uint32_t addr;  /**< any address in the block, host byte order */
  uint8_t prefix; /**< 0 to 32 */
} cidr_t;

/** One entry in a forwarding table. */
typedef struct {
  cidr_t dest;            /**< destination block; host bits must be zero */
  uint32_t gateway;       /**< next hop, or 0 when the block is directly connected */
  char dev[DEV_NAME_LEN]; /**< outgoing interface, for example "eth0" */
} route_t;

/** A forwarding table. Its layout is private to lab.c. */
typedef struct fwd_table fwd_table_t;

/* ------------------------------------------------------------------------
 * Task 1 - Address parsing and formatting
 * --------------------------------------------------------------------- */

/**
 * @brief Parse a dotted quad such as "192.168.1.10".
 *
 * Exactly four decimal octets from 0 to 255 separated by single periods, with
 * nothing before, between or after them. An octet with a leading zero ("010")
 * is rejected, because inet_aton() reads it as octal and that is never what a
 * person typing it meant.
 *
 * @param text The string to parse.
 * @param addr Where the address is stored on success; untouched on failure.
 * @return true if text is a valid dotted quad, false otherwise (including NULL).
 */
bool ip_parse(const char *text, uint32_t *addr);

/**
 * @brief Format an address as a dotted quad.
 * @param addr The address, host byte order.
 * @param out A buffer of at least IP_STR_LEN bytes.
 */
void ip_format(uint32_t addr, char out[IP_STR_LEN]);

/**
 * @brief Parse a block in CIDR notation such as "192.168.1.0/24".
 *
 * The address follows the rules of ip_parse() and the prefix is a decimal
 * number from 0 to 32 with no leading zero. The slash and the prefix are
 * required. Host bits may be set ("132.178.227.11/25" is an interface
 * address), so use cidr_is_network() when they must not be.
 *
 * @param text The string to parse.
 * @param block Where the block is stored on success; untouched on failure.
 * @return true if text is valid CIDR notation, false otherwise (including NULL).
 */
bool cidr_parse(const char *text, cidr_t *block);

/**
 * @brief Format a block as "a.b.c.d/p", using the address exactly as stored.
 *
 * A prefix over 32 is written as 32, the way cidr_mask() treats it.
 *
 * @param block The block.
 * @param out A buffer of at least CIDR_STR_LEN bytes.
 */
void cidr_format(cidr_t block, char out[CIDR_STR_LEN]);

/* ------------------------------------------------------------------------
 * Task 2 - Subnet arithmetic
 * --------------------------------------------------------------------- */

/**
 * @brief The subnet mask for a prefix length: /24 is 255.255.255.0.
 *
 * Careful with /0. Shifting a 32 bit value by 32 is undefined behavior in C.
 *
 * @param prefix 0 to 32; anything larger is treated as 32.
 * @return The mask, host byte order.
 */
uint32_t cidr_mask(uint8_t prefix);

/** @brief The lowest address in the block: the address with the host bits cleared. */
uint32_t cidr_network(cidr_t block);

/** @brief The highest address in the block: the address with the host bits set. */
uint32_t cidr_broadcast(cidr_t block);

/**
 * @brief The first address that can be given to a host.
 *
 * For /0 to /30 that is the network address plus one. A /31 is a point to
 * point link (RFC 3021) where both addresses are usable, and a /32 is a single
 * host, so for those two the first host is the network address.
 */
uint32_t cidr_first_host(cidr_t block);

/**
 * @brief The last address that can be given to a host.
 *
 * The broadcast address minus one for /0 to /30, and the broadcast address
 * itself for /31 and /32 (see cidr_first_host()).
 */
uint32_t cidr_last_host(cidr_t block);

/**
 * @brief How many addresses in the block can be given to hosts.
 *
 * 2^(32 - prefix) - 2 for /0 to /30, 2 for /31, and 1 for /32. For a /0 the
 * 2^32 in that formula needs 33 bits, more than a uint32_t holds, which is why
 * this returns a uint64_t.
 */
uint64_t cidr_host_count(cidr_t block);

/** @brief true if addr falls inside the block. */
bool cidr_contains(cidr_t block, uint32_t addr);

/** @brief true if the block's host bits are all zero, so it names a network. */
bool cidr_is_network(cidr_t block);

/**
 * @brief The i-th of n equally sized subnets of a block.
 *
 * Splitting a /24 into 4 gives four /26 blocks, and subnet 0 is the lowest.
 * Equal sizes are only possible when n is a power of two, and the new prefix
 * can not pass 32.
 *
 * @param block The block to split; it must be a network (cidr_is_network()).
 * @param n How many subnets, a power of two from 1 up.
 * @param i Which subnet, 0 to n - 1.
 * @param out Where the subnet is stored on success; untouched on failure.
 * @return false if out is NULL, the block is not a network, n is not a power
 *         of two, the split would need a prefix longer than 32, or i is out of
 *         range.
 */
bool cidr_subnet(cidr_t block, uint32_t n, uint32_t i, cidr_t *out);

/* ------------------------------------------------------------------------
 * Task 3 - Longest prefix match
 * --------------------------------------------------------------------- */

/**
 * @brief Parse one line of a forwarding table file.
 *
 * A line is three fields separated by spaces or tabs:
 *
 *     <destination> <gateway> <device>
 *
 * The destination is a block in CIDR notation with no host bits set, or the
 * word "default" for 0.0.0.0/0. The gateway is a dotted quad, or the word
 * "direct" for a directly connected block (stored as 0). The device is 1 to
 * DEV_NAME_LEN - 1 characters. Leading and trailing whitespace, including a
 * trailing newline, is ignored. Comments are not handled here.
 *
 * @param line The line.
 * @param route Where the route is stored on success; untouched on failure.
 * @return true if the line is a valid route, false otherwise (including NULL).
 */
bool route_parse(const char *line, route_t *route);

/**
 * @brief Create an empty forwarding table.
 * @return The table, which the caller frees with table_destroy(), or NULL if
 *         memory could not be allocated.
 */
fwd_table_t *table_create(void);

/** @brief Free a table and everything in it. NULL is allowed and does nothing. */
void table_destroy(fwd_table_t *table);

/** @brief The number of routes in the table; 0 for NULL. */
size_t table_count(const fwd_table_t *table);

/**
 * @brief Add a copy of a route to the table, growing it as needed.
 * @return false if either argument is NULL, the destination has host bits
 *         set, the table already holds a route to the same block, or memory
 *         could not be allocated. The table is unchanged on failure.
 */
bool table_add(fwd_table_t *table, const route_t *route);

/**
 * @brief Read a forwarding table file and add every route in it.
 *
 * Everything from a '#' to the end of a line is a comment, and a line that is
 * blank once the comment is removed is skipped. Every other line must be a
 * valid route (route_parse()) that table_add() accepts.
 *
 * @param table The table to add to.
 * @param in The open file to read.
 * @param bad_line If not NULL, set to the 1-based number of the first line
 *        that failed, or 0 if the arguments were NULL or the file could not
 *        be read.
 * @return true if every line loaded. On false, routes from the lines before
 *         the bad one stay in the table.
 */
bool table_load(fwd_table_t *table, FILE *in, size_t *bad_line);

/**
 * @brief Longest prefix match: the route whose block contains dest and has the
 *        longest prefix.
 *
 * A default route (0.0.0.0/0) matches everything, so it is chosen only when
 * nothing more specific does. The order the routes were added in does not
 * matter.
 *
 * @return The matching route, which stays owned by the table, or NULL if no
 *         route matches (or table is NULL).
 */
const route_t *table_lookup(const fwd_table_t *table, uint32_t dest);

#endif // LAB_H
```

### main.c

```c

#include "lab.h"

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef TEST
#define main main_exclude
#endif

static void usage(FILE *stream)
{
  fputs("Usage: myapp info <address>/<prefix>\n"
        "       myapp split <network>/<prefix> <count>\n"
        "       myapp lookup <table-file> <address>...\n", stream);
}

static int command_info(const char *text)
{
  cidr_t block;
  if (!cidr_parse(text, &block)) {
    fprintf(stderr, "myapp: %s: not a valid CIDR block\n", text);
    return 1;
  }
  char address[IP_STR_LEN], mask[IP_STR_LEN], network[IP_STR_LEN], broadcast[IP_STR_LEN], first[IP_STR_LEN], last[IP_STR_LEN];
  ip_format(block.addr, address);
  ip_format(cidr_mask(block.prefix), mask);
  ip_format(cidr_network(block), network);
  ip_format(cidr_broadcast(block), broadcast);
  ip_format(cidr_first_host(block), first);
  ip_format(cidr_last_host(block), last);
  printf("address    %s\nprefix     /%u\nnetmask    %s\nnetwork    %s\nbroadcast  %s\n"
         "first host %s\nlast host  %s\nhosts      %" PRIu64 "\n",
         address, (unsigned)block.prefix, mask, network, broadcast, first, last,
         cidr_host_count(block));
  return 0;
}

static int command_split(const char *text, const char *count_text)
{
  cidr_t block;
  char *end = NULL;
  unsigned long parsed = strtoul(count_text, &end, 10);
  if (!cidr_parse(text, &block) || *count_text == '\0' || *end != '\0' ||
      parsed > UINT32_MAX || parsed == 0) {
    fprintf(stderr, "myapp: invalid split arguments\n");
    return 1;
  }
  uint32_t count = (uint32_t)parsed;
  for (uint32_t index = 0; index < count; index++) {
    cidr_t subnet;
    if (!cidr_subnet(block, count, index, &subnet)) {
      fprintf(stderr, "myapp: cannot split %s into %" PRIu32 " subnets\n", text, count);
      return 1;
    }
    char formatted[19];
    cidr_format(subnet, formatted);
    puts(formatted);
  }
  return 0;
}

static int command_lookup(const char *filename, int address_count, char **addresses)
{
  FILE *file = fopen(filename, "r");
  if (file == NULL) {
    fprintf(stderr, "myapp: %s: %s\n", filename, strerror(errno));
    return 2;
  }
  fwd_table_t *table = table_create();
  size_t bad_line = 0;
  if (table == NULL || !table_load(table, file, &bad_line)) {
    if (bad_line != 0) fprintf(stderr, "myapp: %s:%zu: not a valid route\n", filename, bad_line);
    else fprintf(stderr, "myapp: unable to load %s\n", filename);
    table_destroy(table);
    fclose(file);
    return 2;
  }
  fclose(file);
  int status = 0;
  for (int i = 0; i < address_count; i++) {
    uint32_t address;
    if (!ip_parse(addresses[i], &address)) {
      fprintf(stderr, "myapp: %s: not a valid address\n", addresses[i]);
      status = 1;
      continue;
    }
    const route_t *route = table_lookup(table, address);
    if (route == NULL) {
      puts("unreachable");
      if (status == 0) status = 3;
      continue;
    }
    if (route->gateway != 0) {
      char gateway[IP_STR_LEN];
      ip_format(route->gateway, gateway);
      printf("%s via %s dev %s\n", addresses[i], gateway, route->dev);
    } else {
      printf("%s dev %s\n", addresses[i], route->dev);
    }
  }
  table_destroy(table);
  return status == 1 ? 1 : status;
}

int main(int argc, char **argv)
{
  if (argc == 1) {
    usage(stdout);
    return 0;
  }
  if (strcmp(argv[1], "info") == 0 && argc == 3) return command_info(argv[2]);
  if (strcmp(argv[1], "split") == 0 && argc == 4) return command_split(argv[2], argv[3]);
  if (strcmp(argv[1], "lookup") == 0 && argc >= 4) return command_lookup(argv[2], argc - 3, &argv[3]);
  usage(stderr);
  return 1;
}

```

## Tests Files
### lab-test.c

```c

#include <stdio.h>
#include <string.h>

#include "harness/unity.h"
#include "../src/lab.h"

void setUp(void) {}
void tearDown(void) {}

static cidr_t block(const char *text)
{
  cidr_t result;
  TEST_ASSERT_TRUE(cidr_parse(text, &result));
  return result;
}

void test_ip_parse_and_format(void)
{
  uint32_t address;
  char formatted[16];
  TEST_ASSERT_TRUE(ip_parse("0.0.0.0", &address));
  TEST_ASSERT_EQUAL_UINT32(0, address);
  ip_format(address, formatted);
  TEST_ASSERT_EQUAL_STRING("0.0.0.0", formatted);
  TEST_ASSERT_TRUE(ip_parse("255.255.255.255", &address));
  ip_format(address, formatted);
  TEST_ASSERT_EQUAL_STRING("255.255.255.255", formatted);
  const char *bad[] = {"1.2.3", "1.2.3.4.5", "1..2.3", "256.1.1.1",
                       "010.0.0.1", "1.2.3.4x", "+1.2.3.4", ""};
  for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); i++)
    TEST_ASSERT_FALSE(ip_parse(bad[i], &address));
  TEST_ASSERT_FALSE(ip_parse(NULL, &address));
  TEST_ASSERT_FALSE(ip_parse("1.2.3.4", NULL));
}

void test_cidr_parse_and_format(void)
{
  cidr_t parsed = block("132.178.227.11/25");
  char formatted[19];
  cidr_format(parsed, formatted);
  TEST_ASSERT_EQUAL_STRING("132.178.227.11/25", formatted);
  /* a single-digit prefix exercises the other half of the width branch */
  cidr_format(block("10.0.0.0/8"), formatted);
  TEST_ASSERT_EQUAL_STRING("10.0.0.0/8", formatted);
  const char *bad[] = {"1.2.3.4/33", "1.2.3.4/024", "1.2.3.4/",
                       "1.2.3/24", "1.2.3.4/1x", "1.2.3.4/5/6",
                       "1234567890123456/24"};
  for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); i++)
    TEST_ASSERT_FALSE(cidr_parse(bad[i], &parsed));
  TEST_ASSERT_FALSE(cidr_parse(NULL, &parsed));
  TEST_ASSERT_FALSE(cidr_parse("1.2.3.4/24", NULL));
}

void test_cidr_arithmetic_boundaries(void)
{
  cidr_t subnet = block("132.178.227.11/25");
  TEST_ASSERT_EQUAL_UINT32(0xffffff80, cidr_mask(25));
  TEST_ASSERT_EQUAL_UINT32(0, cidr_mask(0));
  TEST_ASSERT_EQUAL_UINT32(UINT32_MAX, cidr_mask(32));
  TEST_ASSERT_EQUAL_UINT32(0x84b2e300, cidr_network(subnet));
  TEST_ASSERT_EQUAL_UINT32(0x84b2e37f, cidr_broadcast(subnet));
  TEST_ASSERT_EQUAL_UINT32(0x84b2e301, cidr_first_host(subnet));
  TEST_ASSERT_EQUAL_UINT32(0x84b2e37e, cidr_last_host(subnet));
  TEST_ASSERT_EQUAL_UINT32(126, cidr_host_count(subnet));
  TEST_ASSERT_TRUE(cidr_contains(subnet, 0x84b2e332));
  TEST_ASSERT_FALSE(cidr_contains(subnet, 0x84b2e3c8));
  TEST_ASSERT_FALSE(cidr_is_network(subnet));
  TEST_ASSERT_TRUE(cidr_is_network(block("0.0.0.0/0")));

  cidr_t p31 = block("192.0.2.4/31");
  TEST_ASSERT_EQUAL_UINT32(2, cidr_host_count(p31));
  TEST_ASSERT_EQUAL_UINT32(0xc0000204, cidr_first_host(p31));
  TEST_ASSERT_EQUAL_UINT32(0xc0000205, cidr_last_host(p31));
  cidr_t p32 = block("255.255.255.255/32");
  TEST_ASSERT_EQUAL_UINT32(1, cidr_host_count(p32));
  TEST_ASSERT_EQUAL_UINT32(UINT32_MAX, cidr_first_host(p32));
  TEST_ASSERT_EQUAL_UINT32(UINT32_MAX, cidr_last_host(p32));
}

void test_cidr_subnet(void)
{
  cidr_t result;
  cidr_t parent = block("192.168.1.0/24");
  TEST_ASSERT_TRUE(cidr_subnet(parent, 4, 0, &result));
  TEST_ASSERT_EQUAL_UINT32(26, result.prefix);
  TEST_ASSERT_EQUAL_UINT32(0xc0a80100, result.addr);
  TEST_ASSERT_TRUE(cidr_subnet(parent, 4, 3, &result));
  TEST_ASSERT_EQUAL_UINT32(0xc0a801c0, result.addr);
  TEST_ASSERT_FALSE(cidr_subnet(block("192.168.1.5/24"), 4, 0, &result));
  TEST_ASSERT_FALSE(cidr_subnet(parent, 3, 0, &result));
  TEST_ASSERT_FALSE(cidr_subnet(parent, 4, 4, &result));
  TEST_ASSERT_FALSE(cidr_subnet(parent, 4, 0, NULL));
  /* splitting a /32 would need a prefix past 32, so it must be refused */
  TEST_ASSERT_FALSE(cidr_subnet(block("10.0.0.0/32"), 2, 0, &result));
  /* n == 1 is a legal power of two: the block is returned unchanged */
  TEST_ASSERT_TRUE(cidr_subnet(parent, 1, 0, &result));
  TEST_ASSERT_EQUAL_UINT32(24, result.prefix);
}

static route_t route(const char *text)
{
  route_t result;
  TEST_ASSERT_TRUE(route_parse(text, &result));
  return result;
}

void test_routes_and_table(void)
{
  route_t default_route = route("default 132.178.227.1 eno1");
  TEST_ASSERT_NOT_EQUAL_UINT32(0, default_route.gateway);
  route_t direct = route("10.1.2.0/24 direct eth2");
  TEST_ASSERT_EQUAL_UINT32(0, direct.gateway);
  TEST_ASSERT_FALSE(route_parse("10.1.2.0/24 nope eth2", &direct));
  /* destination is three fields but not "default" and not a valid block */
  TEST_ASSERT_FALSE(route_parse("not.an.ip direct eth0", &direct));
  /* destination parses as CIDR but has host bits set, so it is not a network */
  TEST_ASSERT_FALSE(route_parse("10.1.2.5/24 direct eth0", &direct));
  TEST_ASSERT_FALSE(route_parse(NULL, &direct));
  TEST_ASSERT_FALSE(route_parse("default 1.1.1.1 eth0", NULL));
  /* a line longer than route_parse's internal buffer is rejected */
  char huge[300];
  memset(huge, 'x', sizeof(huge) - 1);
  huge[sizeof(huge) - 1] = '\0';
  TEST_ASSERT_FALSE(route_parse(huge, &direct));
  direct = route("10.1.2.0/24 direct eth2");
  fwd_table_t *table = table_create();
  TEST_ASSERT_NOT_NULL(table);
  TEST_ASSERT_TRUE(table_add(table, &default_route));
  TEST_ASSERT_TRUE(table_add(table, &direct));
  TEST_ASSERT_FALSE(table_add(table, &direct));
  /* same address, different prefix, is a different block and must be accepted */
  route_t longer = route("10.1.2.0/25 10.1.2.1 eth2");
  TEST_ASSERT_TRUE(table_add(table, &longer));
  TEST_ASSERT_EQUAL_UINT32(3, table_count(table));
  const route_t *found = table_lookup(table, 0x0a010205);  /* 10.1.2.5, /24 only */
  TEST_ASSERT_NOT_NULL(found);
  TEST_ASSERT_EQUAL_STRING("eth2", found->dev);
  found = table_lookup(table, 0x08080808);  /* 8.8.8.8, default route */
  TEST_ASSERT_EQUAL_STRING("eno1", found->dev);
  /* NULL route and a NULL table are both rejected by table_add */
  TEST_ASSERT_FALSE(table_add(table, NULL));
  TEST_ASSERT_FALSE(table_add(NULL, &direct));
  table_destroy(table);
}

/* The longest prefix must win regardless of the order routes were added. */
void test_longest_prefix_independent_of_order(void)
{
  uint32_t dest = 0x0a0102c8;  /* 10.1.2.200 */
  const char *lines[] = {
    "10.0.0.0/8 direct eth0",
    "10.1.0.0/16 direct eth1",
    "10.1.2.0/24 direct eth2",
    "10.1.2.128/25 10.1.2.1 eth3",
  };
  size_t n = sizeof(lines) / sizeof(lines[0]);
  /* forward order */
  fwd_table_t *a = table_create();
  for (size_t i = 0; i < n; i++) {
    route_t r = route(lines[i]);
    TEST_ASSERT_TRUE(table_add(a, &r));
  }
  const route_t *fa = table_lookup(a, dest);
  TEST_ASSERT_NOT_NULL(fa);
  TEST_ASSERT_EQUAL_UINT32(25, fa->dest.prefix);
  TEST_ASSERT_EQUAL_STRING("eth3", fa->dev);
  table_destroy(a);
  /* reverse order, same answer */
  fwd_table_t *b = table_create();
  for (size_t i = n; i-- > 0;) {
    route_t r = route(lines[i]);
    TEST_ASSERT_TRUE(table_add(b, &r));
  }
  const route_t *fb = table_lookup(b, dest);
  TEST_ASSERT_NOT_NULL(fb);
  TEST_ASSERT_EQUAL_UINT32(25, fb->dest.prefix);
  table_destroy(b);
}

/* Growing past the initial capacity exercises the realloc grow path, and a
 * table with no default route returns NULL for an address nothing covers. */
void test_table_growth_and_no_default(void)
{
  fwd_table_t *table = table_create();
  char line[64];
  for (unsigned i = 0; i < 12; i++) {
    snprintf(line, sizeof(line), "10.%u.0.0/16 direct eth0", i);
    route_t r = route(line);
    TEST_ASSERT_TRUE(table_add(table, &r));
  }
  TEST_ASSERT_EQUAL_UINT32(12, table_count(table));
  /* inside 10.3.0.0/16 */
  TEST_ASSERT_NOT_NULL(table_lookup(table, 0x0a030001));
  /* no default route, so an unrelated address does not match */
  TEST_ASSERT_NULL(table_lookup(table, 0xc0a80001));  /* 192.168.0.1 */
  table_destroy(table);
}

/* NULL arguments are documented as safe no-ops or zero returns. */
void test_table_null_safety(void)
{
  TEST_ASSERT_EQUAL_UINT32(0, table_count(NULL));
  TEST_ASSERT_NULL(table_lookup(NULL, 0));
  size_t line = 123;
  TEST_ASSERT_FALSE(table_load(NULL, NULL, &line));
  TEST_ASSERT_EQUAL_UINT32(0, line);
  table_destroy(NULL);  /* must not crash */
}

/* table_add independently rejects a destination that is not a network, even
 * when the route_t is built by hand rather than parsed. */
void test_table_add_rejects_host_bits(void)
{
  fwd_table_t *table = table_create();
  route_t bogus = {0};
  bogus.dest.addr = 0x0a010205;  /* 10.1.2.5, host bits set */
  bogus.dest.prefix = 24;
  bogus.gateway = 0;
  snprintf(bogus.dev, sizeof(bogus.dev), "eth0");
  TEST_ASSERT_FALSE(table_add(table, &bogus));
  TEST_ASSERT_EQUAL_UINT32(0, table_count(table));
  table_destroy(table);
}

void test_table_load_bad_line(void)
{
  FILE *file = tmpfile();
  TEST_ASSERT_NOT_NULL(file);
  /* a comment, a blank line, a trailing comment, then a bad line in the middle */
  fputs("# comment\n\n10.0.0.0/8 direct eth0  # ok\nbad line\n", file);
  rewind(file);
  fwd_table_t *table = table_create();
  size_t line = 0;
  TEST_ASSERT_FALSE(table_load(table, file, &line));
  TEST_ASSERT_EQUAL_UINT32(4, line);
  TEST_ASSERT_EQUAL_UINT32(1, table_count(table));
  table_destroy(table);
  fclose(file);
}

/* A completely valid file loads every route and leaves bad_line at zero. */
void test_table_load_success(void)
{
  FILE *file = tmpfile();
  TEST_ASSERT_NOT_NULL(file);
  fputs("default 10.0.0.1 eth0\n10.0.0.0/8 direct eth0\n", file);
  rewind(file);
  fwd_table_t *table = table_create();
  size_t line = 99;
  TEST_ASSERT_TRUE(table_load(table, file, &line));
  TEST_ASSERT_EQUAL_UINT32(0, line);
  TEST_ASSERT_EQUAL_UINT32(2, table_count(table));
  table_destroy(table);
  fclose(file);
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_ip_parse_and_format);
  RUN_TEST(test_cidr_parse_and_format);
  RUN_TEST(test_cidr_arithmetic_boundaries);
  RUN_TEST(test_cidr_subnet);
  RUN_TEST(test_routes_and_table);
  RUN_TEST(test_longest_prefix_independent_of_order);
  RUN_TEST(test_table_growth_and_no_default);
  RUN_TEST(test_table_null_safety);
  RUN_TEST(test_table_add_rejects_host_bits);
  RUN_TEST(test_table_load_bad_line);
  RUN_TEST(test_table_load_success);
  return UNITY_END();
}

```

## Scripts Files
Report generated on 11/02/2026 at 01:41:47


---

## End of Report

SHA-256 Hash of the report: 8a36fbd141ae4d6c3963ea8e4ee7204642a3ab8e3b4b625460a1ca684aadcd7c

Do not edit the generated report. Any changes will be reported as academic dishonesty

---
## GitHub Info
- GitHub repo name: SethMcDaniel547/cs425-p3
- The repository visibility is public.
- The workflow was triggered by SethMcDaniel547
