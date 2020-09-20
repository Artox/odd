/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
#include <assert.h>

#ifdef __linux__
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#endif

#include "dd_storage.h"
#include <dd_types.h>

/*
 * Declare Tables
 */
struct table {
  void *base;
  size_t length;
  size_t rowsize;
};
typedef struct table table;
table bindings_table = {0};
table reports_table = {0};

struct table_entry {
  uint8_t valid;
  uint8_t eid;
  uint16_t cid;
  char data[];
};
typedef struct table_entry table_entry;

/*
 * Platform Implementations
 */

#ifdef __linux__
/* File Backed Storage */
static int fd = -1;
static void *map_base_hint = (void *)0x12000000;
static void *map_base = 0;

int dd_storage_init(dd_device *device) {
  // open data file
  fd = open("data.bin", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
  if (fd == -1) {
    // print error once
    perror(0);
    return -1;
  }

  // ensure size
  int ret = posix_fallocate(fd, 0, 64 * 1024);
  if (ret != 0) {
    fprintf(stderr, "%s\n", strerror(ret));
    // expect undefined behaviour
    return -1;
  }

  // map to fixed virtual address
  if (map_base == 0) {
    map_base =
        mmap(map_base_hint, 64 * 1024, PROT_READ | PROT_WRITE | PROT_EXEC,
             MAP_FIXED | MAP_SHARED, fd, 0);
    if (map_base == 0) {
      // print error
      perror(0);
      return -1;
    }
  }

  // partition space among tables
  bindings_table.base = map_base;
  bindings_table.length = 32;
  bindings_table.rowsize = 1024; // TODO: estimate reasonable row size
  reports_table.base =
      bindings_table.base + bindings_table.length * bindings_table.rowsize;
  reports_table.length = 32;
  reports_table.rowsize = 1024; // TODO: estimate reasonable row size

  // future: fix virtual addresses by storing previous in file
  assert(map_base == map_base_hint);

  // done
  return 0;
}
#endif

/*
 * Accessors
 */

static void *dd_storage_put(table *table, uint8_t eid, uint16_t cid,
                            size_t *index, void *data, size_t size,
                            void (*copy)(void *, size_t, void *)) {
  assert(table != 0);
  assert(index != 0);

  if (table->rowsize < sizeof(table_entry) + size) {
    // exceeds row size
    return 0;
  }

  // find free slot
  assert(table->base != 0);
  table_entry *slot = table->base;
  *index = 0;
  while (*index < table->length) {
    if (slot->valid != 1)
      break;

    slot += table->rowsize;
    (*index)++;
  }
  if (*index >= table->length) {
    // no free slot
    return 0;
  }

  // copy into table
  assert(copy != 0);
  copy(slot->data, table->rowsize - sizeof(table_entry), data);

  // mark valid
  slot->eid = eid;
  slot->cid = cid;
  slot->valid = 1;

  // return pointer to data
  return slot->data;
}

static void *dd_storage_get(table *table, int index, uint8_t *eid,
                            uint16_t *cid) {
  assert(table != 0);
  table_entry *slot = table->base + index * table->rowsize;

  if (slot->valid == 1) {
    *eid = slot->eid;
    *cid = slot->cid;
    return slot->data;
  }
  return 0;
}

dd_binding *dd_storage_bindings_put(uint8_t eid, uint16_t cid,
                                    dd_binding *binding) {
  size_t index;
  dd_binding *result =
      dd_storage_put(&bindings_table, eid, cid, &index, binding,
                     sizeof(dd_binding) + binding->length,
                     (void (*)(void *, size_t, void *))dd_copy_binding);
  if (result == 0) {
    // oom
    return 0;
  }

  // derive id from index
  result->id = index + 1;

  // return pointer to storage
  return result;
}

dd_binding *dd_storage_bindings_update(dd_binding *orig, dd_binding *updated) {
  assert(orig != 0);
  assert(updated != 0);

  // TODO: revisit regarding non-mmap storage implementations ...
  return dd_copy_binding(orig, bindings_table.rowsize, updated);
}

void dd_storage_bindings_delete(dd_binding *orig) {
  assert(orig != 0);

  // TODO: revisit regarding non-mmap storage implementations ...
  table_entry *entry = (void *)orig - sizeof(table_entry);
  entry->valid = 0;
}

dd_report *dd_storage_reports_put(uint8_t eid, uint16_t cid,
                                  dd_report *report) {
  assert(report != 0);
  size_t index;
  dd_report *result =
      dd_storage_put(&reports_table, eid, cid, &index, report,
                     sizeof(dd_report) + report->length,
                     (void (*)(void *, size_t, void *))dd_copy_report);
  if (result == 0) {
    // oom
    return 0;
  }

  // derive id from index
  result->id = index + 1;

  // return pointer to storage
  return result;
}

dd_report *dd_storage_reports_update(dd_report *orig, dd_report *updated) {
  assert(orig != 0);
  assert(updated != 0);

  // TODO: revisit regarding non-mmap storage implementations ...
  return dd_copy_report(orig, reports_table.rowsize, updated);
}

void dd_storage_reports_delete(dd_report *orig) {
  assert(orig != 0);

  // TODO: revisit regarding non-mmap storage implementations ...
  table_entry *entry = (void *)orig - sizeof(table_entry);
  entry->valid = 0;
}

/*
 * link tables into resource tree
 */
static void dd_storage_link_bindings(dd_device *device) {
  assert(device != 0);

  // link resource tree (bindings)
  for (size_t i = 0; i < bindings_table.length; i++) {
    uint8_t eid;
    uint16_t cid;

    // really bad algorithm right here ... ... ... !
    dd_binding *binding = dd_storage_get(&bindings_table, i, &eid, &cid);
    if (binding != 0) {
      // report configuration exists
      assert(device != 0);
      for (int j = 0; j < device->endpoints_length; j++) {
        dd_endpoint *endpoint = device->endpoints[j];
        if (endpoint->id == eid) {
          for (int k = 0; k < endpoint->cluster_length; k++) {
            dd_cluster *cluster = endpoint->cluster[k];
            if (cluster->id == cid) {
              assert(cluster->bindings_length < DD_CLUSTER_BINDINGS_MAX);
              cluster->bindings[cluster->bindings_length++] = binding;
              goto dd_storage_init__nextbinding;
            }
          }
        }
      }
    }
  dd_storage_init__nextbinding:;
  }
}

static void dd_storage_link_reports(dd_device *device) {
  assert(device != 0);

  // link resource tree (reports)
  for (size_t i = 0; i < reports_table.length; i++) {
    uint8_t eid;
    uint16_t cid;

    // really bad algorithm right here ... ... ... !
    dd_report *report = dd_storage_get(&reports_table, i, &eid, &cid);
    if (report != 0) {
      // report configuration exists
      assert(device != 0);
      for (int j = 0; j < device->endpoints_length; j++) {
        dd_endpoint *endpoint = device->endpoints[i];
        if (endpoint->id == eid) {
          for (int k = 0; k < endpoint->cluster_length; k++) {
            dd_cluster *cluster = endpoint->cluster[k];
            if (cluster->id == cid) {
              assert(cluster->reports_length < DD_CLUSTER_REPORTS_MAX);
              cluster->reports[cluster->reports_length++] = report;
              goto dd_storage_init__nextreport;
            }
          }
        }
      }
    }
  dd_storage_init__nextreport:;
  }
}

void dd_storage_link(dd_device *device) {
  dd_storage_link_bindings(device);
  dd_storage_link_reports(device);
}
