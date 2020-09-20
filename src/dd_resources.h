/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
#ifndef HAVE_DDRESOURCES_H
#define HAVE_DDRESOURCES_H

#include <coap2/coap.h>

/*
 * Generic Resource Handlers
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
struct dd_report;
typedef struct dd_report dd_report;

// DELETE/GET/POST/PUT /
void dd_handle_root(coap_context_t *context, struct coap_resource_t *resource,
                    coap_session_t *session, coap_pdu_t *request,
                    coap_binary_t *token, coap_string_t *query,
                    coap_pdu_t *response);

// GET /zcl
void dd_handle_zcl_get(struct coap_resource_t *resource,
                       coap_session_t *session, coap_pdu_t *request,
                       coap_binary_t *token, coap_string_t *query,
                       coap_pdu_t *response);

// GET /zcl/e
void dd_handle_endpoints_get(dd_device *device,
                             struct coap_resource_t *resource,
                             coap_session_t *session, coap_pdu_t *request,
                             coap_binary_t *token, coap_string_t *query,
                             coap_pdu_t *response);

// GET /zcl/e/<eid>
void dd_handle_endpoint_get(dd_device *device, dd_endpoint *endpoint,
                            struct coap_resource_t *resource,
                            coap_session_t *session, coap_pdu_t *request,
                            coap_binary_t *token, coap_string_t *query,
                            coap_pdu_t *response);

// GET /zcl/e/<eid>/<cl>
void dd_handle_cluster_get(dd_device *device, dd_endpoint *endpoint,
                           dd_cluster *cluster,
                           struct coap_resource_t *resource,
                           coap_session_t *session, coap_pdu_t *request,
                           coap_binary_t *token, coap_string_t *query,
                           coap_pdu_t *response);

// GET /zcl/e/<eid>/<cl>/a
void dd_handle_attributes_get(dd_device *device, dd_endpoint *endpoint,
                              dd_cluster *cluster,
                              struct coap_resource_t *resource,
                              coap_session_t *session, coap_pdu_t *request,
                              coap_binary_t *token, coap_string_t *query,
                              coap_pdu_t *response);

// POST /zcl/e/<eid>/<cl>/a
void dd_handle_attributes_post(dd_device *device, dd_endpoint *endpoint,
                               dd_cluster *cluster,
                               struct coap_resource_t *resource,
                               coap_session_t *session, coap_pdu_t *request,
                               coap_binary_t *token, coap_string_t *query,
                               coap_pdu_t *response);

// GET /zcl/e/<eid>/<cl>/a/<aid>
void dd_handle_attribute_get(dd_device *device, dd_endpoint *endpoint,
                             dd_cluster *cluster, dd_attribute *attribute,
                             struct coap_resource_t *resource,
                             coap_session_t *session, coap_pdu_t *request,
                             coap_binary_t *token, coap_string_t *query,
                             coap_pdu_t *response);

// PUT /zcl/e/<eid>/<cl>/a/<aid>
void dd_handle_attribute_put(dd_device *device, dd_endpoint *endpoint,
                             dd_cluster *cluster, dd_attribute *attribute,
                             struct coap_resource_t *resource,
                             coap_session_t *session, coap_pdu_t *request,
                             coap_binary_t *token, coap_string_t *query,
                             coap_pdu_t *response);

// GET /zcl/e/<eid>/<cl>/b
void dd_handle_bindings_get(dd_device *device, dd_endpoint *endpoint,
                            dd_cluster *cluster,
                            struct coap_resource_t *resource,
                            coap_session_t *session, coap_pdu_t *request,
                            coap_binary_t *token, coap_string_t *query,
                            coap_pdu_t *response);

// GET /zcl/e/<eid>/<cl>/<bid>
void dd_handle_binding_get(dd_device *device, dd_endpoint *endpoint,
                           dd_cluster *cluster, dd_binding *binding,
                           struct coap_resource_t *resource,
                           coap_session_t *session, coap_pdu_t *request,
                           coap_binary_t *token, coap_string_t *query,
                           coap_pdu_t *response);

// PUT /zcl/e/<eid>/<cl>/<bid>
void dd_handle_binding_put(dd_device *device, dd_endpoint *endpoint,
                           dd_cluster *cluster, dd_binding *binding,
                           struct coap_resource_t *resource,
                           coap_session_t *session, coap_pdu_t *request,
                           coap_binary_t *token, coap_string_t *query,
                           coap_pdu_t *response);

// DELETE /zcl/e/<eid>/<cl>/<bid>
void dd_handle_binding_delete(dd_device *device, dd_endpoint *endpoint,
                              dd_cluster *cluster, dd_binding *binding,
                              struct coap_resource_t *resource,
                              coap_session_t *session, coap_pdu_t *request,
                              coap_binary_t *token, coap_string_t *query,
                              coap_pdu_t *response);

// POST /zcl/e/<eid>/<cl>/b
void dd_handle_bindings_post(dd_device *device, dd_endpoint *endpoint,
                             dd_cluster *cluster,
                             struct coap_resource_t *resource,
                             coap_session_t *session, coap_pdu_t *request,
                             coap_binary_t *token, coap_string_t *query,
                             coap_pdu_t *response);

// GET /zcl/e/<eid>/<cl>/c
void dd_handle_commands_get(dd_device *device, dd_endpoint *endpoint,
                            dd_cluster *cluster,
                            struct coap_resource_t *resource,
                            coap_session_t *session, coap_pdu_t *request,
                            coap_binary_t *token, coap_string_t *query,
                            coap_pdu_t *response);

// POST /zcl/e/<eid>/<cl>/c/<cid>
void dd_handle_command_post(dd_device *device, dd_endpoint *endpoint,
                            dd_cluster *cluster, dd_command *command,
                            struct coap_resource_t *resource,
                            coap_session_t *session, coap_pdu_t *request,
                            coap_binary_t *token, coap_string_t *query,
                            coap_pdu_t *response);

// POST /zcl/e/<eid>/<cl>/n
void dd_handle_notification_post(dd_device *device, dd_endpoint *endpoint,
                                 dd_cluster *cluster,
                                 struct coap_resource_t *resource,
                                 coap_session_t *session, coap_pdu_t *request,
                                 coap_binary_t *token, coap_string_t *query,
                                 coap_pdu_t *response);

// GET /zcl/e/<eid>/<cl>/r
void dd_handle_reports_get(dd_device *device, dd_endpoint *endpoint,
                           dd_cluster *cluster,
                           struct coap_resource_t *resource,
                           coap_session_t *session, coap_pdu_t *request,
                           coap_binary_t *token, coap_string_t *query,
                           coap_pdu_t *response);

// POST /zcl/e/<eid>/<cl>/r
void dd_handle_reports_post(dd_device *device, dd_endpoint *endpoint,
                            dd_cluster *cluster,
                            struct coap_resource_t *resource,
                            coap_session_t *session, coap_pdu_t *request,
                            coap_binary_t *token, coap_string_t *query,
                            coap_pdu_t *response);

// GET /zcl/e/<eid>/<cl>/r/<rid>
void dd_handle_report_get(dd_device *device, dd_endpoint *endpoint,
                          dd_cluster *cluster, dd_report *report,
                          struct coap_resource_t *resource,
                          coap_session_t *session, coap_pdu_t *request,
                          coap_binary_t *token, coap_string_t *query,
                          coap_pdu_t *response);

// PUT /zcl/e/<eid>/<cl>/r/<rid>
void dd_handle_report_put(dd_device *device, dd_endpoint *endpoint,
                          dd_cluster *cluster, dd_report *report,
                          struct coap_resource_t *resource,
                          coap_session_t *session, coap_pdu_t *request,
                          coap_binary_t *token, coap_string_t *query,
                          coap_pdu_t *response);

// DELETE /zcl/e/<eid>/<cl>/r/<rid>
void dd_handle_report_delete(dd_device *device, dd_endpoint *endpoint,
                             dd_cluster *cluster, dd_report *report,
                             struct coap_resource_t *resource,
                             coap_session_t *session, coap_pdu_t *request,
                             coap_binary_t *token, coap_string_t *query,
                             coap_pdu_t *response);

/*
 * Periodic Jobs
 */
int32_t dd_process_bindings(coap_context_t *context, dd_device *device);

/*
 * Shortcuts
 */

// json_object *dd_response_default(uint8_t code);

#endif /* HAVE_DDRESOURCES_H */
