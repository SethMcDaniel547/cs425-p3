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
