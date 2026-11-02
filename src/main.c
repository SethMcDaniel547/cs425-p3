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
