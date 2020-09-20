/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
#ifndef HAVE_DDTYPES_H
#define HAVE_DDTYPES_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

/*
 * ZCL Resources
 */

struct dd_attribute;
typedef struct dd_attribute dd_attribute;
struct dd_binding;
typedef struct dd_binding dd_binding;
struct dd_cluster;
typedef struct dd_cluster dd_cluster;
struct dd_command;
typedef struct dd_command dd_command;
struct dd_device;
typedef struct dd_device dd_device;
struct dd_endpoint;
typedef struct dd_endpoint dd_endpoint;
struct dd_notification;
typedef struct dd_notification dd_notification;
struct dd_report;
typedef struct dd_report dd_report;
struct dd_report_attribute;
typedef struct dd_report_attribute dd_report_attribute;
enum dd_scheme;
typedef enum dd_scheme dd_scheme;
struct dd_uri;
typedef struct dd_uri dd_uri;
enum dd_value_type;
typedef enum dd_value_type dd_value_type;
struct dd_value;
typedef struct dd_value dd_value;

typedef dd_value *(*dd_attribute_read_handler)(void *buffer,
                                               size_t buffer_size);
typedef void (*dd_attribute_write_handler)(dd_value *value);
typedef void (*dd_command_handler)();
typedef void (*dd_notification_handler)(dd_notification *notification);

struct dd_attribute {
  // each attribute has a unique id
  uint16_t id;
  // each attribute has a name
  const char *name;

  // each attribute has read and write handlers
  dd_attribute_read_handler read;
  dd_attribute_write_handler write;
};

struct dd_binding {
  // each binding has a unique id
  uint8_t id;

  // each binding has a destination uri
  dd_uri *uri;
  // each binding has a report identifier
  uint8_t rid;

  // timestamp of last notification
  time_t timestamp;

  unsigned int length; // number of bytes appended in _buffer
  char _buffer[];
};

#define DD_CLUSTER_BINDINGS_MAX 16
#define DD_CLUSTER_REPORTS_MAX 4
struct dd_cluster {
  // each cluster instance has a unique id
  uint16_t id;
  // each cluster instance has a role
  char role; // [c|s]
  // each cluster instance can optionally have a specific manufacturer id
  uint16_t manufacturer;

  // a cluster contains attributes
  dd_attribute **attributes;
  size_t attributes_length;
  // and bindings (dynamic)
  dd_binding *bindings[DD_CLUSTER_BINDINGS_MAX];
  size_t bindings_length;
  // and commands
  dd_command **commands;
  size_t commands_length;
  // and report configurations (dynamic)
  dd_report *reports[DD_CLUSTER_REPORTS_MAX];
  size_t reports_length;
  // and (optional) notification handler
  dd_notification_handler notify;
};

struct dd_command {
  // each command has a unique id
  uint16_t id;

  // each command has a handler
  dd_command_handler exec;
};

struct dd_device {
  // a device contains endpoints
  dd_endpoint **endpoints;
  size_t endpoints_length;
};

struct dd_endpoint {
  // each endpoint has a unique id
  uint8_t
      id; // ZCL-IP spec section 2.1.3 hints that endpoint identifiers are uint8

  // an endpoint contains clusters
  dd_cluster **cluster;
  size_t cluster_length;
};

struct dd_notification {
  // TODO: attributes
  uint8_t bid; // binding id
  uint8_t rid; // report configuration id
  time_t timestamp;
  dd_uri *uri; // source uri
};

struct dd_report {
  // each report configuration has a unique id
  uint8_t id;

  // each report configuration has minimum and maximum reporting intervals in
  // seconds
  uint16_t min_reporting_interval;
  uint16_t max_reporting_interval;

  // each report configuration has an attribute configuration
  dd_report_attribute *attributes;
  size_t attributes_length;

  size_t length;  // number of bytes appended in _buffer
  char _buffer[]; // storage for variable sized (optional) members
};

struct dd_report_attribute {
  uint16_t aid;

  // note: thresholds can only be analog types (int,uint,float,time)
  dd_value *reportable_change;
  dd_value *low_threshold;
  dd_value *high_threshold;

  unsigned int length; // number of bytes appended in _buffer
  char _buffer[];
};

enum dd_scheme {
  DD_NONE = 0,
  DD_COAP = 1,
  DD_COAPS = 2,
};

struct dd_uri {
  dd_scheme scheme;
  const char *host;
  uint16_t port;
  const char *path;
  unsigned int length; // number of bytes appended in _buffer
  char _buffer[];
};

enum dd_value_type {
  DD_BOOL,
  DD_INT,
  DD_UINT,
  DD_TIME,
  DD_STRING,
};

struct dd_value {
  dd_value_type type;  // base data type
  unsigned int length; // number of bytes appended in _buffer

  union dd_value_value {
    bool vbool;
    int64_t vint;
    uint64_t vuint;
    time_t vtime;
    const char *vstring;
  } value;
  char _buffer[]; // storage for dynamic size values (string)
};

extern dd_device *__device; // root of ZCL Resource Tree

dd_binding *dd_copy_binding(void *destination, size_t destination_size,
                            dd_binding *source);
dd_report *dd_copy_report(void *destination, size_t destination_size,
                          dd_report *source);
dd_report_attribute *dd_copy_report_attribute(void *destination,
                                              size_t destination_size,
                                              dd_report_attribute *source);
dd_uri *dd_copy_uri(void *destination, size_t destination_size, dd_uri *source);
dd_value *dd_copy_value(void *destination, size_t destination_size,
                        dd_value *source);

bool dd_value_to_bool(dd_value *value);
dd_value *dd_bool_to_value(bool vbool, void *buffer, size_t buffer_size);

int8_t dd_value_to_int8(dd_value *value);
dd_value *dd_int8_to_value(int8_t vint, void *buffer, size_t buffer_size);
int16_t dd_value_to_int16(dd_value *value);
dd_value *dd_int16_to_value(int16_t vint, void *buffer, size_t buffer_size);
int32_t dd_value_to_int32(dd_value *value);
dd_value *dd_int32_to_value(int32_t vint, void *buffer, size_t buffer_size);

uint8_t dd_value_to_uint8(dd_value *value);
dd_value *dd_uint8_to_value(uint8_t vuint, void *buffer, size_t buffer_size);
uint16_t dd_value_to_uint16(dd_value *value);
dd_value *dd_uint16_to_value(uint16_t vuint, void *buffer, size_t buffer_size);
uint32_t dd_value_to_uint32(dd_value *value);
dd_value *dd_uint32_to_value(uint32_t vuint, void *buffer, size_t buffer_size);

time_t dd_value_to_UTC(dd_value *value);
dd_value *dd_UTC_to_value(time_t vtime, void *buffer, size_t buffer_size);

const char *dd_value_to_string(dd_value *value);
dd_value *dd_string_to_value(const char *value, void *buffer,
                             size_t buffer_size);

#endif /* HAVE_DDTYPES_H */
