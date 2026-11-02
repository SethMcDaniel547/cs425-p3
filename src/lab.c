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
