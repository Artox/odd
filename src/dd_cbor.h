/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
#ifndef HAVE_DDCBOR_H
#define HAVE_DDCBOR_H

#include <qcbor/qcbor.h>
#include <stdint.h>

struct dd_report_attribute;
typedef struct dd_report_attribute dd_report_attribute;
struct dd_uri;
typedef struct dd_uri dd_uri;
struct dd_value;
typedef struct dd_value dd_value;

// add value to map with key
void dd_cbor_add_value_key(QCBOREncodeContext *ctx, const char *key,
                           dd_value *value);

// add value to map with numeric key
void dd_cbor_add_value_keyn(QCBOREncodeContext *ctx, int64_t key,
                            dd_value *value);

// add uri to map with key
void dd_cbor_add_uri_key(QCBOREncodeContext *ctx, const char *key, dd_uri *uri);

/*
 * get report attribute configuration items from cbor map
 *
 * returns:
 * -  0 on error
 * -  pointer to start of result data list
 */
dd_report_attribute *dd_cbor_get_report_attribute_configurations(
    QCBORDecodeContext *ctx, uint16_t nentries, void *buffer,
    size_t buffer_size, size_t *buffer_length);

/*
 * get uri from cbor string
 *
 * returns:
 * -  0 on error
 * -  pointer to dd_uri with result data
 */
dd_uri *dd_cbor_get_uri(QCBORItem *source, void *buffer, int buffer_size);

/*
 * get value object from cbor object
 *
 * returns:
 * -  0 on error
 * -  pointer to dd_value with result data
 */
dd_value *dd_cbor_get_value(QCBORItem *source, void *buffer, int buffer_size);

#endif /* HAVE_DDCBOR_H */
