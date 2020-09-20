/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dd_cbor.h"
#include "dd_types.h"

void dd_cbor_add_value_key(QCBOREncodeContext *ctx, const char *key,
                           dd_value *value) {
  assert(ctx != 0);
  assert(value != 0);

  switch (value->type) {
  case DD_BOOL:
    QCBOREncode_AddBoolToMap(ctx, key, value->value.vbool);
    break;
  case DD_INT:
    QCBOREncode_AddInt64ToMap(ctx, key, value->value.vint);
    break;
  case DD_UINT:
    QCBOREncode_AddUInt64ToMap(ctx, key, value->value.vuint);
    break;
  case DD_STRING:
    QCBOREncode_AddSZStringToMap(ctx, key, value->value.vstring);
    break;
  case DD_TIME:
    QCBOREncode_AddDateEpochToMap(ctx, key, value->value.vtime);
    break;
  default:
    assert(0);
  }
}

void dd_cbor_add_value_keyn(QCBOREncodeContext *ctx, int64_t key,
                            dd_value *value) {
  assert(ctx != 0);
  assert(value != 0);

  switch (value->type) {
  case DD_BOOL:
    QCBOREncode_AddBoolToMapN(ctx, key, value->value.vbool);
    break;
  case DD_INT:
    QCBOREncode_AddInt64ToMapN(ctx, key, value->value.vint);
    break;
  case DD_UINT:
    QCBOREncode_AddUInt64ToMapN(ctx, key, value->value.vuint);
    break;
  case DD_STRING:
    QCBOREncode_AddSZStringToMapN(ctx, key, value->value.vstring);
    break;
  case DD_TIME:
    QCBOREncode_AddDateEpochToMapN(ctx, key, value->value.vtime);
    break;
  default:
    assert(0);
  }
}

void dd_cbor_add_uri_key(QCBOREncodeContext *ctx, const char *key,
                         dd_uri *uri) {
  assert(ctx != 0);
  assert(uri != 0);
  const char *scheme_str = 0;
  char uri_str[1024];    // TODO: estimate maximum uri size
  char port_str[7] = {}; // ':' + uint16_max + '\0'

  // optional: <scheme:> prefix
  switch (uri->scheme) {
  case DD_NONE:
    scheme_str = "";
    break;
  case DD_COAP:
    scheme_str = "coap:";
    break;
  case DD_COAPS:
    scheme_str = "coaps:";
    break;
  default:
    assert(0);
  }

  // optional: :<port> suffix
  if (uri->port != 0)
    snprintf(port_str, sizeof(port_str), ":%u", uri->port);

  // generate string
  size_t uri_str_len = snprintf(uri_str, sizeof(uri_str), "%s//%s%s%s",
                                scheme_str, uri->host, port_str, uri->path);
  assert(uri_str_len < sizeof(uri_str));

  // add to cbor map
  QCBOREncode_AddSZStringToMap(ctx, key, uri_str);
}

/*
 * get report attribute configuration item from cbor map
 *
 * returns:
 * -  0 on error
 * -  pointer to dd_report_attribute with result data
 */
static dd_report_attribute *
dd_cbor_get_report_attribute_configuration(QCBORDecodeContext *ctx,
                                           uint16_t nentries, uint16_t aid,
                                           void *buffer, size_t buffer_size) {
  assert(ctx != 0);
  assert(buffer != 0);
  QCBORError cderr;
  QCBORItem item;
  size_t buffer_offset = 0;

  if (buffer_size < sizeof(dd_report_attribute)) {
    // oom
    return 0;
  }
  dd_report_attribute *attribute_configuration = buffer;
  buffer_offset += sizeof(dd_report_attribute);
  bzero(attribute_configuration, sizeof(dd_report_attribute));
  attribute_configuration->aid = aid;

  // Note: 0 entries is a valid configuration!
  for (int i = 0; i < nentries; i++) {
    cderr = QCBORDecode_GetNext(ctx, &item);
    if (cderr != QCBOR_SUCCESS || item.uLabelType != QCBOR_TYPE_TEXT_STRING ||
        item.label.string.len != 1) {
      // expect string key length 1
      return 0;
    }

    // parse value
    dd_value *value = dd_cbor_get_value(&item, buffer + buffer_offset,
                                        buffer_size - buffer_offset);
    if (value == 0) {
      // parsing failed
      return 0;
    }
    buffer_offset += sizeof(dd_value) + value->length;

    switch (((const char *)item.label.string.ptr)[0]) {
    case 'h':
      attribute_configuration->high_threshold = value;
      break;
    case 'l':
      attribute_configuration->low_threshold = value;
      break;
    case 'r':
      attribute_configuration->reportable_change = value;
      break;
    default:
      // unexpected key
      return 0;
    }
  }
  attribute_configuration->length +=
      buffer_offset - sizeof(dd_report_attribute);

  // return result
  return attribute_configuration;
}

dd_report_attribute *dd_cbor_get_report_attribute_configurations(
    QCBORDecodeContext *ctx, uint16_t nentries, void *buffer,
    size_t buffer_size, size_t *buffer_length) {
  assert(ctx != 0);
  assert(buffer != 0);
  assert(buffer_length != 0);
  QCBORError cderr;
  QCBORItem item;
  size_t _buffer_length = 0;
  dd_report_attribute *head = 0;

  for (uint16_t i = 0; i < nentries; i++) {
    cderr = QCBORDecode_GetNext(ctx, &item);
    if (cderr != QCBOR_SUCCESS ||
        item.uLabelType !=
            QCBOR_TYPE_INT64 /* everything <= int64_max reports int64 */
        || 0 > item.label.int64 || item.label.int64 > UINT16_MAX ||
        item.uDataType != QCBOR_TYPE_MAP) {
      // expect map with uint16 key
      return 0;
    }

    uint16_t aid = item.label.int64;
    dd_report_attribute *attribute_configuration =
        dd_cbor_get_report_attribute_configuration(
            ctx, item.val.uCount, aid, buffer + _buffer_length,
            buffer_size - _buffer_length);
    if (attribute_configuration == 0) {
      // parsing failed
      return 0;
    }
    _buffer_length +=
        sizeof(dd_report_attribute) + attribute_configuration->length;

    // set head of list
    if (head == 0)
      head = attribute_configuration;
  }

  // return result head and length
  *buffer_length = _buffer_length;
  return head;
}

dd_uri *dd_cbor_get_uri(QCBORItem *source, void *buffer, int buffer_size) {
  assert(source != 0);
  assert(buffer != 0);
  assert(buffer_size >= sizeof(dd_uri));
  dd_uri *result = buffer;
  buffer_size -= sizeof(dd_uri);
  bzero(buffer, sizeof(dd_uri));
  const char *uri_host = 0, *uri_path = 0;
  int uri_host_len, uri_path_len;
  const char *itr, *end;

  // expect string data
  if (source->uDataType != QCBOR_TYPE_TEXT_STRING) {
    return 0;
  }
  itr = source->val.string.ptr;
  end = itr + source->val.string.len;

  // look for coap(s): prefix (optional)
  if (end - itr >= 4 && itr[0] == 'c' && itr[1] == 'o' && itr[2] == 'a' &&
      itr[3] == 'p') {
    itr += 4;

    // look for secure indicator
    if (itr < end && itr[0] == 's') {
      // have coaps
      itr++;
      result->scheme = DD_COAPS;
    } else {
      // have plain coap
      result->scheme = DD_COAP;
    }

    // look for trailing colon
    if (itr < end && itr[0] == ':') {
      itr++;
    } else {
      // colon is required after URI-Scheme!
      return 0;
    }
  } else {
    // scheme unspecified
    result->scheme = DD_NONE;
  }

  // look for leading "//"
  if (end - itr >= 2 && itr[0] == '/' && itr[1] == '/') {
    itr += 2;
  } else {
    // "//" is required before URI-Host!
    return 0;
  }
  uri_host = itr;

  if (itr < end && itr[0] == '[') {
    // IPv6 address [xxxx:xxxx:xxxx::xxxx]
    // scan URI-Host till ']'
    while (itr < end && itr++[0] != ']')
      ;
  }
  // scan URI-Host till colon or slash
  while (itr < end && itr[0] != ':' && itr[0] != '/') {
    itr++;
  }
  uri_host_len = itr - uri_host;
  if (uri_host_len == 0) {
    // URI-Host is required!
    return 0;
  }

  // copy to result
  if (uri_host_len >= buffer_size) {
    // buffer too small
    return 0;
  }
  result->host = result->_buffer + result->length;
  memcpy((char *)result->host, uri_host, uri_host_len);
  ((char *)result->host)[uri_host_len] = '\0';
  result->length += uri_host_len + 1;
  buffer_size -= uri_host_len + 1;

  // have URI-Port? (optional)
  if (itr < end && itr[0] == ':') {
    itr++;

    unsigned long port = strtoul(itr, (char **)&itr, 10);
    if (itr == 0 || port > UINT16_MAX) {
      // invalid port
      return 0;
    }
    result->port = port;
  } else {
    result->port = 0;
  }

  // URI-Path is last
  if (itr < end && itr[0] != '/') {
    // URI-Path must start with leading '/'
    return 0;
  }
  uri_path = itr;
  uri_path_len = end - uri_path;
  // TODO: is empty URI-Path allowed?

  // copy to result
  if (uri_path_len >= buffer_size) {
    // buffer too small
    return 0;
  }
  result->path = result->_buffer + uri_host_len + 1;
  memcpy((char *)result->path, uri_path, uri_path_len);
  ((char *)result->path)[uri_path_len] = '\0';
  result->length += uri_path_len + 1;
  buffer_size -= uri_path_len + 1;

  // done
  assert(result->host >= result->_buffer);
  assert(result->host < result->_buffer + result->length);
  assert(result->path >= result->_buffer);
  assert(result->path < result->_buffer + result->length);
  assert(result->length == strlen(result->host) + strlen(result->path) + 2);
  return result;
}

dd_value *dd_cbor_get_value(QCBORItem *source, void *buffer, int buffer_size) {
  assert(source != 0);
  assert(buffer != 0);
  assert(buffer_size >= sizeof(dd_value));
  dd_value *value = buffer;
  bzero(buffer, sizeof(dd_value));

  if (source->uDataType == QCBOR_TYPE_TRUE ||
      source->uDataType == QCBOR_TYPE_FALSE) {
    value->type = DD_BOOL;
    value->value.vbool = (source->uDataType == QCBOR_TYPE_TRUE);

    return value;
  } else if (source->uDataType ==
             QCBOR_TYPE_INT64 /* everything <= int64_max reports int64 */) {
    if (source->val.int64 >= 0) {
      value->type = DD_UINT;
      value->value.vuint = source->val.int64;
    } else {
      value->type = DD_INT;
      value->value.vint = source->val.int64;
    }

    return value;
  } else if (source->uDataType == QCBOR_TYPE_TEXT_STRING) {
    if (buffer_size <= sizeof(dd_value) + source->val.string.len) {
      // oom
      return 0;
    }

    value->type = DD_STRING;
    value->value.vstring = value->_buffer;
    memcpy(value->_buffer, source->val.string.ptr, source->val.string.len);
    value->_buffer[source->val.string.len] = 0;
    value->length = source->val.string.len + 1;

    return value;
  } else if (source->uDataType == QCBOR_TYPE_DATE_EPOCH) {
    value->type = DD_TIME;
    value->value.vtime = source->val.epochDate.nSeconds;

    return value;
  }
  // TODO: implement other types

  return 0;
}
