/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
#include <assert.h>
#include <string.h>

#include "dd_types.h"

static void fix_binding(dd_binding *new, dd_binding *orig);
static void fix_report(dd_report *new, dd_report *orig);
static void fix_report_attribute(dd_report_attribute *new,
                                 dd_report_attribute *orig);
static void fix_uri(dd_uri *new, dd_uri *orig);
static void fix_value(dd_value *new, dd_value *orig);

static void fix_binding(dd_binding *new, dd_binding *orig) {
  if (orig->uri != 0) {
    assert((void *)orig->uri > (void *)orig &&
           (void *)orig->uri < (void *)orig->_buffer + orig->length);
    new->uri = (void *)orig->uri - (void *)orig->_buffer + (void *)new->_buffer;
    fix_uri(new->uri, orig->uri);
  }
}

static void fix_report(dd_report *new, dd_report *orig) {
  if (orig->attributes != 0) {
    assert((void *)orig->attributes > (void *)orig &&
           (void *)orig->attributes < (void *)orig->_buffer + orig->length);
    new->attributes =
        (void *)orig->attributes - (void *)orig->_buffer + (void *)new->_buffer;
    for (size_t i = 0; i < orig->attributes_length;) {
      fix_report_attribute(new->attributes + i, orig->attributes + i);
      i += sizeof(dd_report_attribute) + (orig->attributes + i)->length;
    }
  }
}

static void fix_report_attribute(dd_report_attribute *new,
                                 dd_report_attribute *orig) {
  if (orig->reportable_change != 0) {
    assert((void *)orig->reportable_change > (void *)orig &&
           (void *)orig->reportable_change <
               (void *)orig->_buffer + orig->length);
    new->reportable_change = (void *)orig->reportable_change -
                             (void *)orig->_buffer + (void *)new->_buffer;
    fix_value(new->reportable_change, orig->reportable_change);
  }
  if (orig->low_threshold != 0) {
    assert((void *)orig->low_threshold > (void *)orig &&
           (void *)orig->low_threshold < (void *)orig->_buffer + orig->length);
    new->low_threshold = (void *)orig->low_threshold - (void *)orig->_buffer +
                         (void *)new->_buffer;
    fix_value(new->low_threshold, orig->low_threshold);
  }
  if (orig->high_threshold) {
    assert((void *)orig->high_threshold > (void *)orig &&
           (void *)orig->high_threshold < (void *)orig->_buffer + orig->length);
    new->high_threshold = (void *)orig->high_threshold - (void *)orig->_buffer +
                          (void *)new->_buffer;
    fix_value(new->high_threshold, orig->high_threshold);
  }
}

static void fix_uri(dd_uri *new, dd_uri *orig) {
  assert((void *)orig->host > (void *)orig &&
         (void *)orig->host < (void *)orig->_buffer + orig->length);
  new->host = (void *)orig->host - (void *)orig->_buffer + (void *)new->_buffer;
  assert((void *)orig->path > (void *)orig &&
         (void *)orig->path < (void *)orig->_buffer + orig->length);
  new->path = (void *)orig->path - (void *)orig->_buffer + (void *)new->_buffer;
}

static void fix_value(dd_value *new, dd_value *orig) {
  if (orig->value.vstring != 0) {
    new->value.vstring = (void *)orig->value.vstring - (void *)orig->_buffer +
                         (void *)new->_buffer;
  }
}

dd_binding *dd_copy_binding(void *destination, size_t destination_size,
                            dd_binding *source) {
  assert(source != 0);

  if (destination_size < sizeof(dd_binding) + source->length) {
    // oom
    return 0;
  }

  // memcpy + fix pointers
  memcpy(destination, source, sizeof(dd_binding) + source->length);
  fix_binding(destination, source);

  return destination;
}

dd_report *dd_copy_report(void *destination, size_t destination_size,
                          dd_report *source) {
  assert(source != 0);

  if (destination_size < sizeof(dd_report) + source->length) {
    // oom
    return 0;
  }

  // memcpy + fix pointers
  memcpy(destination, source, sizeof(dd_report) + source->length);
  fix_report(destination, source);

  return destination;
}

dd_report_attribute *dd_copy_report_attribute(void *destination,
                                              size_t destination_size,
                                              dd_report_attribute *source) {
  assert(source != 0);

  if (destination_size < sizeof(dd_report_attribute) + source->length) {
    // oom
    return 0;
  }

  // memcpy + fix pointers
  memcpy(destination, source, sizeof(dd_report_attribute) + source->length);
  fix_report_attribute(destination, source);

  return destination;
}

dd_uri *dd_copy_uri(void *destination, size_t destination_size,
                    dd_uri *source) {
  assert(source != 0);

  if (destination_size < sizeof(dd_uri) + source->length) {
    // oom
    return 0;
  }

  // memcpy + fix pointers
  memcpy(destination, source, sizeof(dd_report_attribute) + source->length);
  fix_uri(destination, source);

  return destination;
}

dd_value *dd_copy_value(void *destination, size_t destination_size,
                        dd_value *source) {
  assert(source != 0);

  if (destination_size < sizeof(dd_value) + source->length) {
    // oom
    return 0;
  }

  // memcpy + fix pointers
  memcpy(destination, source, sizeof(dd_value) + source->length);
  fix_value(destination, source);

  return destination;
}

bool dd_value_to_bool(dd_value *value) {
  assert(value != 0);
  assert(value->type == DD_BOOL);

  return value->value.vbool;
}

dd_value *dd_bool_to_value(bool vbool, void *buffer, size_t buffer_size) {
  assert(buffer != 0);
  assert(buffer_size >= sizeof(dd_value));

  dd_value *value = buffer;
  bzero(value, sizeof(dd_value));
  value->type = DD_BOOL; // TODO:
  value->value.vbool = vbool;

  return value;
}

static int32_t dd_value_to_int(dd_value *value, int32_t min, int32_t max) {
  assert(value != 0);

  if (value->type == DD_INT) {
    assert(min <= value->value.vint && value->value.vint <= max);
    return value->value.vint;
  } else if (value->type == DD_UINT) {
    assert(value->value.vuint <= max);
    return value->value.vuint;
  }

  assert(0);
  return 0;
}

static dd_value *dd_int_to_value(int32_t vint, void *buffer,
                                 size_t buffer_size) {
  assert(buffer != 0);
  assert(buffer_size >= sizeof(dd_value));

  dd_value *value = buffer;
  bzero(value, sizeof(dd_value));
  value->type = DD_INT;
  value->value.vint = vint;

  return value;
}

int8_t dd_value_to_int8(dd_value *value) {
  return dd_value_to_int(value, INT8_MIN, INT8_MAX);
}

dd_value *dd_int8_to_value(int8_t vint, void *buffer, size_t buffer_size) {
  return dd_int_to_value(vint, buffer, buffer_size);
}

int16_t dd_value_to_int16(dd_value *value) {
  return dd_value_to_int(value, INT16_MIN, INT16_MAX);
}

dd_value *dd_int16_to_value(int16_t vint, void *buffer, size_t buffer_size) {
  return dd_int_to_value(vint, buffer, buffer_size);
}

int32_t dd_value_to_int32(dd_value *value) {
  return dd_value_to_int(value, INT32_MIN, INT32_MAX);
}

dd_value *dd_int32_to_value(int32_t vint, void *buffer, size_t buffer_size) {
  return dd_int_to_value(vint, buffer, buffer_size);
}

static uint32_t dd_value_to_uint(dd_value *value, uint32_t max) {
  assert(value != 0);

  if (value->type == DD_INT) {
    assert(0 <= value->value.vint && value->value.vint <= max);
    return value->value.vint;
  } else if (value->type == DD_UINT) {
    assert(value->value.vuint <= max);
    return value->value.vuint;
  }

  assert(0);
  return 0;
}

static dd_value *dd_uint_to_value(uint32_t vuint, void *buffer,
                                  size_t buffer_size) {
  assert(buffer != 0);
  assert(buffer_size >= sizeof(dd_value));

  dd_value *value = buffer;
  bzero(value, sizeof(dd_value));
  value->type = DD_UINT;
  value->value.vuint = vuint;

  return value;
}

uint8_t dd_value_to_uint8(dd_value *value) {
  return dd_value_to_uint(value, UINT8_MAX);
}

dd_value *dd_uint8_to_value(uint8_t vuint, void *buffer, size_t buffer_size) {
  return dd_uint_to_value(vuint, buffer, buffer_size);
}

uint16_t dd_value_to_uint16(dd_value *value) {
  return dd_value_to_uint(value, UINT16_MAX);
}

dd_value *dd_uint16_to_value(uint16_t vuint, void *buffer, size_t buffer_size) {
  return dd_uint_to_value(vuint, buffer, buffer_size);
}

uint32_t dd_value_to_uint32(dd_value *value) {
  return dd_value_to_uint(value, UINT32_MAX);
}

dd_value *dd_uint32_to_value(uint32_t vuint, void *buffer, size_t buffer_size) {
  return dd_uint_to_value(vuint, buffer, buffer_size);
}

time_t dd_value_to_UTC(dd_value *value) {
  assert(value != 0);
  assert(value->type == DD_TIME);

  return value->value.vtime;
}

dd_value *dd_UTC_to_value(time_t vtime, void *buffer, size_t buffer_size) {
  assert(buffer != 0);
  assert(buffer_size >= sizeof(dd_value));

  dd_value *value = buffer;
  bzero(value, sizeof(dd_value));
  value->type = DD_TIME;
  value->value.vtime = vtime;

  return value;
}

const char *dd_value_to_string(dd_value *value) {
  assert(value != 0);
  assert(value->type == DD_STRING);

  return value->value.vstring;
}

dd_value *dd_string_to_value(const char *vstring, void *buffer,
                             size_t buffer_size) {
  assert(buffer != 0);
  assert(buffer_size >= sizeof(dd_value));
  assert(vstring != 0);

  dd_value *value = buffer;
  bzero(value, sizeof(dd_value));
  value->type = DD_STRING;
  value->value.vstring = value->_buffer;
  value->length = strlen(vstring) + 1;
  if (sizeof(dd_value) + value->length > buffer_size) {
    // oom
    return 0;
  }
  memcpy(value->_buffer, vstring, value->length);

  return value;
}
