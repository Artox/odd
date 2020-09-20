/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
#ifndef HAVE_DDSTORAGE_H
#define HAVE_DDSTORAGE_H

#include <stdint.h>

struct dd_device;
typedef struct dd_device dd_device;
struct dd_binding;
typedef struct dd_binding dd_binding;
struct dd_report;
typedef struct dd_report dd_report;

/*
 * ZCL Persistent Storage Abstraction Layer
 */

int dd_storage_init();
void dd_storage_link(dd_device *device);

/*
 * Binding Table
 */
dd_binding *dd_storage_bindings_put(uint8_t eid, uint16_t cid,
                                    dd_binding *binding);
dd_binding *dd_storage_bindings_get(int index, uint8_t *eid, uint16_t *cid);
dd_binding *dd_storage_bindings_update(dd_binding *orig, dd_binding *updated);
void dd_storage_bindings_delete(dd_binding *orig);

/*
 * Report Configuration Table
 */
dd_report *dd_storage_reports_put(uint8_t eid, uint16_t cid, dd_report *report);
dd_report *dd_storage_reports_get(int index, uint8_t *eid, uint16_t *cid);
dd_report *dd_storage_reports_update(dd_report *orig, dd_report *updated);
void dd_storage_reports_delete(dd_report *orig);

#endif /* HAVE_DDSTORAGE_H */
