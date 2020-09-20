/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
#include <ctype.h>
#include <stdio.h>

#include "dd_cbor.h"
#include "dd_coap.h"
#include "dd_resources.h"
#include "dd_storage.h"
#include "dd_types.h"

/*
 * strtoul helper
 *
 * returns:
 * -  ptr to next character after numeric expression on success
 * -  0 on error
 */
static const char *dd_strtoul(unsigned long int *destination,
                              const char *source, int base,
                              unsigned long int max) {
  assert(destination != 0 && source != 0);
  char *end;

  *destination = strtoul(source, &end, base);
  if ((*destination == 0 && source[0] != '0') || end[0] != '\0' ||
      max < *destination) {
    // parsing failed, was incomplete or value out of range
    return 0;
  }

  return end;
}

/**
 * decode a percent-encoded C string with optional path normalization
 *
 * The buffer pointed to by @dst must be at least strlen(@src) bytes.
 * Decoding stops at the first character from @src that decodes to null.
 *
 * @param dst       destination buffer
 * @param src       source buffer

 * @return          number of valid characters in @dst
 * @author          Johan Lindh <johan@linkdata.se>
 * @legalese        BSD licensed (http://opensource.org/licenses/BSD-2-Clause)
 */
static ptrdiff_t urldecode(char *dst, char *src) {
  char *org_dst = dst;
  char ch, a, b;
  do {
    ch = *src++;
    if (ch == '%' && isxdigit(a = src[0]) && isxdigit(b = src[1])) {
      if (a < 'A')
        a -= '0';
      else if (a < 'a')
        a -= 'A' - 10;
      else
        a -= 'a' - 10;
      if (b < 'A')
        b -= '0';
      else if (b < 'a')
        b -= 'A' - 10;
      else
        b -= 'a' - 10;
      ch = 16 * a + b;
      src += 2;
    }
    *dst++ = ch;
  } while (ch);
  return (dst - org_dst) - 1;
}

/*
 * Generic Handler for all Requests
 */
void dd_handle_root(coap_context_t *context, struct coap_resource_t *resource,
                    coap_session_t *session, coap_pdu_t *request,
                    coap_binary_t *token, coap_string_t *query,
                    coap_pdu_t *response) {
  coap_string_t *path;
  const char *level;
  dd_device *device = __device;
  dd_endpoint *endpoint = 0;
  dd_cluster *cluster = 0;
  dd_attribute *attribute = 0;
  dd_binding *binding = 0;
  dd_command *command = 0;
  dd_report *report = 0;

  // look-up request path
  path = coap_get_uri_path(request);
  assert(path !=
         0); // code study shows this is always a valid coap_string_t instance

  // somehow uri path might be urlencoded :@
  urldecode((char *)path->s, (char *)path->s);

  printf("invoked root handler at \"%s\"\n", path->s);
  goto uri;

uri: // /
  // parse path - first level
  level = strtok((char *)path->s, "/");

  //  only know about "zcl" at this time
  if (level == 0 || strcmp("zcl", level) != 0) {
    printf("first level not \"zcl\"\n");
    goto error_no_resource;
  }

  goto uri_zcl;

uri_zcl: // /zcl
  // parse path - second level - zcl/
  level = strtok(0, "/");
  if (level == 0) {
    // Entrypoint Resources
    if (request->code == COAP_REQUEST_GET) {
      dd_handle_zcl_get(resource, session, request, token, query, response);
      goto end_no_bias;
    } else {
      goto error_no_resource;
    }
  }

  if (strcmp("e", level) == 0) {
    goto uri_endpoint;
  }

  // TODO: other entrypoint resources
  goto error_no_resource;

uri_endpoint:          // /zcl/e
  assert(device != 0); // parent resource must exist

  // parse next level, if any
  level = strtok(0, "/");

  if (level == 0) {
    // Endpoint Collection
    if (request->code == COAP_REQUEST_GET) {
      dd_handle_endpoints_get(device, resource, session, request, token, query,
                              response);
      goto end_no_bias;
    } else {
      goto error_no_resource;
    }
  }

  // expect Endpoint Identifier here
  unsigned long int eid; // endpoint identifiers are limited by 1 byte
  if (dd_strtoul(&eid, level, 16, UINT8_MAX) == 0) {
    printf("invalid endpoint identifier \"%s\"\n", level);
    goto error_no_resource;
  }

  // find this endpoint in resource tree
  for (size_t i = 0; i < device->endpoints_length; i++) {
    endpoint = device->endpoints[i];
    if (endpoint->id == eid) {
      goto uri_endpoint_eid;
    }

    // try next endpoint
    continue;
  }

  // no such endpoint :(
  goto error_no_resource;

uri_endpoint_eid:        // /zcl/e/<eid>
  assert(endpoint != 0); // parent resource must exist

  // check path on this level
  level = strtok(0, "/");

  if (level == 0) {
    // Endpoint Resource Collection
    switch (request->code) {
    case COAP_REQUEST_GET:
      dd_handle_endpoint_get(device, endpoint, resource, session, request,
                             token, query, response);
      goto end_no_bias;
    case COAP_REQUEST_POST:
    case COAP_REQUEST_PUT:
    case COAP_REQUEST_DELETE:
      response->code = COAP_RESPONSE_CODE(405); // method not allowed
      goto end_no_bias;
    default:
      assert(0); // other methods don't propagate here
    }
  }

  // expect cluster identifier here
  // first cluster role (c|s)
  char role = level[0];
  if (role != 'c' && role != 's') {
    printf("invalid cluster role \"%s\"\n", level);
    goto error_no_resource;
  }

  // Note: '\0' would have been caught as invalid role in previous step -->
  // strlen(level) >= 1 next cluster number
  unsigned long int cl; // cluster numbers are limited by 2 byte
  if (dd_strtoul(&cl, &level[1], 16, UINT16_MAX) == 0) {
    printf("invalid cluster number \"%s\"\n", &level[1]);
    goto error_no_resource;
  }

  // find this cluster in resource tree
  for (size_t i = 0; i < endpoint->cluster_length; i++) {
    cluster = endpoint->cluster[i];
    if (cluster->id == cl && cluster->role == role) {
      goto uri_endpoint_eid_cl;
    }

    // try next cluster
    continue;
  }

  // no such cluster :(
  goto error_no_resource;

uri_endpoint_eid_cl:    // /zcl/e/<eid>/<cl>
  assert(cluster != 0); // parent resource must exist

  // check path on this level
  level = strtok(0, "/");

  if (level == 0) {
    // Cluster Resource Collection
    switch (request->code) {
    case COAP_REQUEST_GET:
      dd_handle_cluster_get(device, endpoint, cluster, resource, session,
                            request, token, query, response);
      goto end_no_bias;
    case COAP_REQUEST_POST:
    case COAP_REQUEST_PUT:
    case COAP_REQUEST_DELETE:
      response->code = COAP_RESPONSE_CODE(405); // method not allowed
      goto end_no_bias;
    default:
      assert(0); // other methods don't propagate here
    }
  }

  if (strcmp("a", level) == 0) {
    goto uri_endpoint_eid_cl_attribute;
  }

  if (strcmp("b", level) == 0) {
    goto uri_endpoint_eid_cl_binding;
  }

  if (strcmp("c", level) == 0) {
    goto uri_endpoint_eid_cl_command;
  }

  if (strcmp("n", level) == 0) {
    goto uri_endpoint_eid_cl_notification;
  }

  if (strcmp("r", level) == 0) {
    goto uri_endpoint_eid_cl_reports;
  }

  // this resource has no further children
  goto error_no_resource;

uri_endpoint_eid_cl_attribute: // /zcl/e/<eid>/<cl>/a
  assert(cluster != 0);        // parent resource must exist

  // check path on this level
  level = strtok(0, "/");

  if (level == 0) {
    // Attribute Collection
    switch (request->code) {
    case COAP_REQUEST_GET:
      dd_handle_attributes_get(device, endpoint, cluster, resource, session,
                               request, token, query, response);
      goto end_no_bias;
    case COAP_REQUEST_POST:
      dd_handle_attributes_post(device, endpoint, cluster, resource, session,
                                request, token, query, response);
      goto end_no_bias;
    case COAP_REQUEST_PUT:
    case COAP_REQUEST_DELETE:
      response->code = COAP_RESPONSE_CODE(405); // method not allowed
      goto end_no_bias;
    default:
      assert(0); // other methods don't propagate here
    }
  }

  // expect attribute identifier here
  unsigned long int aid; // attribute identifiers are limited by 2 byte
  if (dd_strtoul(&aid, level, 16, UINT16_MAX) == 0) {
    printf("invalid attribute identifier \"%s\"\n", level);
    goto error_no_resource;
  }

  // find this attribute in resource tree
  for (size_t i = 0; i < cluster->attributes_length; i++) {
    attribute = cluster->attributes[i];
    if (attribute->id == aid) {
      goto uri_endpoint_eid_cl_attribute_aid;
    }

    // try next attribute
    continue;
  }

  // no such attribute :(
  goto error_no_resource;

uri_endpoint_eid_cl_attribute_aid: // /zcl/e/<eid>/<cl>/a/<aid>
  assert(attribute != 0);          // parent resource must exist

  // check path on this level
  level = strtok(0, "/");

  if (level == 0) {
    // Attribute Instance
    switch (request->code) {
    case COAP_REQUEST_GET:
      dd_handle_attribute_get(device, endpoint, cluster, attribute, resource,
                              session, request, token, query, response);
      goto end_no_bias;
    case COAP_REQUEST_PUT:
      dd_handle_attribute_put(device, endpoint, cluster, attribute, resource,
                              session, request, token, query, response);
      goto end_no_bias;
    case COAP_REQUEST_POST:
    case COAP_REQUEST_DELETE:
      response->code = COAP_RESPONSE_CODE(405); // method not allowed
      goto end_no_bias;
    default:
      assert(0); // other methods don't propagate here
    }
  }

  // this resource has no children
  goto error_no_resource;

uri_endpoint_eid_cl_binding: // /zcl/e/<eid>/<cl>/b
  assert(cluster != 0);      // parent resource must exist

  // check path on this level
  level = strtok(0, "/");

  if (level == 0) {
    // Binding Collection
    switch (request->code) {
    case COAP_REQUEST_GET:
      dd_handle_bindings_get(device, endpoint, cluster, resource, session,
                             request, token, query, response);
      goto end_no_bias;
    case COAP_REQUEST_POST:
      dd_handle_bindings_post(device, endpoint, cluster, resource, session,
                              request, token, query, response);
      goto end_no_bias;
    case COAP_REQUEST_PUT:
    case COAP_REQUEST_DELETE:
      response->code = COAP_RESPONSE_CODE(405); // method not allowed
      goto end_no_bias;
    default:
      assert(0); // other methods don't propagate here
    }
  }

  // expect binding identifier
  unsigned long int bid; // binding identifiers are limited by 1 byte
  if (dd_strtoul(&bid, level, 16, UINT8_MAX) == 0) {
    printf("invalid binding identifier \"%s\"\n", level);
    goto error_no_resource;
  }

  // find this binding in resource tree
  for (size_t i = 0; i < cluster->bindings_length; i++) {
    binding = cluster->bindings[i];
    if (binding->id == bid) {
      goto uri_endpoint_eid_cl_binding_bid;
    }

    // try next binding
    continue;
  }

  // no such binding :(
  goto error_no_resource;

uri_endpoint_eid_cl_binding_bid: // /zcl/e/<eid>/<cl>/b/<bid>
  assert(binding != 0);          // parent resource must exist

  // check path on this level
  level = strtok(0, "/");

  if (level == 0) {
    // Binding Instance
    switch (request->code) {
    case COAP_REQUEST_GET:
      dd_handle_binding_get(device, endpoint, cluster, binding, resource,
                            session, request, token, query, response);
      goto end_no_bias;
    case COAP_REQUEST_PUT:
      dd_handle_binding_put(device, endpoint, cluster, binding, resource,
                            session, request, token, query, response);
      goto end_no_bias;
    case COAP_REQUEST_DELETE:
      dd_handle_binding_delete(device, endpoint, cluster, binding, resource,
                               session, request, token, query, response);
      goto end_no_bias;
    case COAP_REQUEST_POST:
      response->code = COAP_RESPONSE_CODE(405); // method not allowed
      goto end_no_bias;
    default:
      assert(0); // other methods don't propagate here
    }
  }

  // this resource has no children
  goto error_no_resource;

uri_endpoint_eid_cl_command: // /zcl/e/<eid>/<cl>/c
  assert(cluster != 0);      // parent resource must exist

  // check path on this level
  level = strtok(0, "/");

  if (level == 0) {
    // Command Collection
    switch (request->code) {
    case COAP_REQUEST_GET:
      dd_handle_commands_get(device, endpoint, cluster, resource, session,
                             request, token, query, response);
      goto end_no_bias;
    case COAP_REQUEST_POST:
    case COAP_REQUEST_PUT:
    case COAP_REQUEST_DELETE:
      response->code = COAP_RESPONSE_CODE(405); // method not allowed
      goto end_no_bias;
    default:
      assert(0); // other methods don't propagate here
    }
  }

  // expect command identifier here
  unsigned long int cid; // command identifiers are limited by 2 byte
  if (dd_strtoul(&cid, level, 16, UINT16_MAX) == 0) {
    printf("invalid command identifier \"%s\"\n", level);
    goto error_no_resource;
  }

  // find this command in resource tree
  for (size_t i = 0; i < cluster->commands_length; i++) {
    command = cluster->commands[i];
    if (command->id == cid) {
      goto uri_endpoint_eid_cl_command_cid;
    }

    // try next command
    continue;
  }

  // no such command :(
  goto error_no_resource;

uri_endpoint_eid_cl_command_cid: // /zcl/e/<eid>/<cl>/c/<cid>
  assert(command != 0);          // parent resource must exist

  // check path on this level
  level = strtok(0, "/");

  if (level == 0) {
    // Command Instance
    switch (request->code) {
    case COAP_REQUEST_POST:
      dd_handle_command_post(device, endpoint, cluster, command, resource,
                             session, request, token, query, response);
      goto end_no_bias;
    case COAP_REQUEST_GET:
    case COAP_REQUEST_PUT:
    case COAP_REQUEST_DELETE:
      response->code = COAP_RESPONSE_CODE(405); // method not allowed
      goto end_no_bias;
    default:
      assert(0); // other methods don't propagate here
    }
  }

  // this resource has no children
  goto error_no_resource;

uri_endpoint_eid_cl_notification:
  assert(cluster != 0); // parent resource must exist

  // check path on this level
  level = strtok(0, "/");

  if (level == 0) {
    // Notification Endpoint
    switch (request->code) {
    case COAP_REQUEST_POST:
      dd_handle_notification_post(device, endpoint, cluster, resource, session,
                                  request, token, query, response);
      goto end_no_bias;
    case COAP_REQUEST_GET:
    case COAP_REQUEST_PUT:
    case COAP_REQUEST_DELETE:
      response->code = COAP_RESPONSE_CODE(405); // method not allowed
      goto end_no_bias;
    default:
      assert(0); // other methods don't propagate here
    }
  }

  // this resource has no children
  goto error_no_resource;

uri_endpoint_eid_cl_reports: // /zcl/e/<eid>/<cl>/r
  assert(cluster != 0);      // parent resource must exist

  // check path on this level
  level = strtok(0, "/");

  if (level == 0) {
    // Report Configuration Collection
    switch (request->code) {
    case COAP_REQUEST_GET:
      dd_handle_reports_get(device, endpoint, cluster, resource, session,
                            request, token, query, response);
      goto end_no_bias;
    case COAP_REQUEST_POST:
      dd_handle_reports_post(device, endpoint, cluster, resource, session,
                             request, token, query, response);
      goto end_no_bias;
    case COAP_REQUEST_DELETE:
    case COAP_REQUEST_PUT:
      response->code = COAP_RESPONSE_CODE(405); // method not allowed
      goto end_no_bias;
    default:
      assert(0); // other methods don't propagate here
    }
  }

  // expect report identifier here
  unsigned long int rid; // report identifiers are limited by 1 byte
  if (dd_strtoul(&rid, level, 16, UINT8_MAX) == 0) {
    printf("invalid report identifier \"%s\"\n", level);
    goto error_no_resource;
  }

  // find this report configuration in resource tree
  for (size_t i = 0; i < cluster->reports_length; i++) {
    report = cluster->reports[i];
    if (report->id == rid) {
      goto uri_endpoint_eid_cl_report_rid;
    }

    // try next report
    continue;
  }

  // no such report :(
  goto error_no_resource;

uri_endpoint_eid_cl_report_rid:
  assert(report != 0); // parent resource must exist

  // check path on this level
  level = strtok(0, "/");

  if (level == 0) {
    // Attribute Instance
    switch (request->code) {
    case COAP_REQUEST_GET:
      dd_handle_report_get(device, endpoint, cluster, report, resource, session,
                           request, token, query, response);
      goto end_no_bias;
    case COAP_REQUEST_PUT:
      dd_handle_report_put(device, endpoint, cluster, report, resource, session,
                           request, token, query, response);
      goto end_no_bias;
    case COAP_REQUEST_DELETE:
      dd_handle_report_delete(device, endpoint, cluster, report, resource,
                              session, request, token, query, response);
      goto end_no_bias;
    case COAP_REQUEST_POST:
      response->code = COAP_RESPONSE_CODE(405); // method not allowed
      goto end_no_bias;
    default:
      assert(0); // other methods don't propagate here
    }
  }

  // this resource has no children
  goto error_no_resource;

error_no_resource:
  // failure case when requested resource doesn't exist
  // return codes as per ZCL-IP spec section 2.8.6.1.1 table 12
  switch (request->code) {
  case COAP_REQUEST_DELETE:
    response->code = COAP_RESPONSE_CODE(202);
    break;
  case COAP_REQUEST_GET:
    response->code = COAP_RESPONSE_CODE(404);
    break;
  case COAP_REQUEST_POST:
    response->code = COAP_RESPONSE_CODE(404);
    break;
  case COAP_REQUEST_PUT:
    response->code = COAP_RESPONSE_CODE(404);
    break;
  default:
    assert(0);
  }

end_no_bias:
  // return here if response code has been set already
  return;
}

/*
 * Generic Resource Handlers
 */

// GET /zcl
void dd_handle_zcl_get(struct coap_resource_t *resource,
                       coap_session_t *session, coap_pdu_t *request,
                       coap_binary_t *token, coap_string_t *query,
                       coap_pdu_t *response) {
  QCBOREncodeContext cec;
  UsefulBuf_MAKE_STACK_UB(
      response_buffer,
      7); // pre-calculated space for entry-point resources ["e","g","t"]
  UsefulBufC response_result = NULLUsefulBufC;

  QCBOREncode_Init(&cec, response_buffer);
  QCBOREncode_OpenArray(&cec);
  QCBOREncode_AddSZString(&cec, "e");
  // QCBOREncode_AddSZString(&cec, "g"); // TODO: implement
  // QCBOREncode_AddSZString(&cec, "t"); // TODO: implement
  QCBOREncode_CloseArray(&cec);
  QCBORError ceerr = QCBOREncode_Finish(&cec, &response_result);
  assert(ceerr == QCBOR_SUCCESS);

  response->code = COAP_RESPONSE_CODE(205);
  coap_add_data_blocked_response(resource, session, request, response, token,
                                 COAP_MEDIATYPE_APPLICATION_CBOR, -1,
                                 response_result.len, response_result.ptr);
}

// GET /zcl/e
void dd_handle_endpoints_get(dd_device *device,
                             struct coap_resource_t *resource,
                             coap_session_t *session, coap_pdu_t *request,
                             coap_binary_t *token, coap_string_t *query,
                             coap_pdu_t *response) {
  assert(device != 0);
  QCBOREncodeContext cec;
  UsefulBuf_MAKE_STACK_UB(
      response_buffer,
      2 * 256 + 1); // endpoint ids uint8 (0..255) - each max 2 byte
  UsefulBufC response_result = NULLUsefulBufC;

  // encode endpoint identifiers as cbor array
  QCBOREncode_Init(&cec, response_buffer);
  QCBOREncode_OpenArray(&cec);
  for (size_t i = 0; i < device->endpoints_length; i++)
    QCBOREncode_AddUInt64(&cec, device->endpoints[i]->id);
  QCBOREncode_CloseArray(&cec);
  QCBORError ceerr = QCBOREncode_Finish(&cec, &response_result);
  assert(ceerr == QCBOR_SUCCESS);

  // respond with 205 Content, media type application/cbor
  response->code = COAP_RESPONSE_CODE(205);
  coap_add_data_blocked_response(resource, session, request, response, token,
                                 COAP_MEDIATYPE_APPLICATION_CBOR, -1,
                                 response_result.len, response_result.ptr);
}

// GET /zcl/e/<eid>
void dd_handle_endpoint_get(dd_device *device, dd_endpoint *endpoint,
                            struct coap_resource_t *resource,
                            coap_session_t *session, coap_pdu_t *request,
                            coap_binary_t *token, coap_string_t *query,
                            coap_pdu_t *response) {
  assert(device != 0);
  assert(endpoint != 0);
  QCBOREncodeContext cec;
  UsefulBuf_MAKE_STACK_UB(response_buffer,
                          1024); // TODO: estimate meaningful size
  UsefulBufC response_result = NULLUsefulBufC;

  // encode cluster identifiers as cbor array
  QCBOREncode_Init(&cec, response_buffer);
  QCBOREncode_OpenArray(&cec);
  for (int i = 0; i < endpoint->cluster_length; i++) {
    char buf[1 + 4 + 1 + 4 + 1]; // 'c|s' + YYYY + '_' + ZZZZ + '\0'
    int offset = snprintf(buf, sizeof(buf), "%c%x", endpoint->cluster[i]->role,
                          endpoint->cluster[i]->id);
    if (endpoint->cluster[i]->manufacturer != 0) {
      snprintf(buf + offset, sizeof(buf) - offset, "_%x",
               endpoint->cluster[i]->manufacturer);
    }
    QCBOREncode_AddSZString(&cec, buf);
  }
  QCBOREncode_CloseArray(&cec);
  QCBORError ceerr = QCBOREncode_Finish(&cec, &response_result);
  assert(ceerr == QCBOR_SUCCESS);

  // respond with 205 Content, media type application/cbor
  response->code = COAP_RESPONSE_CODE(205);
  coap_add_data_blocked_response(resource, session, request, response, token,
                                 COAP_MEDIATYPE_APPLICATION_CBOR, -1,
                                 response_result.len, response_result.ptr);
}

// GET /zcl/e/<eid>/<cl>
void dd_handle_cluster_get(dd_device *device, dd_endpoint *endpoint,
                           dd_cluster *cluster,
                           struct coap_resource_t *resource,
                           coap_session_t *session, coap_pdu_t *request,
                           coap_binary_t *token, coap_string_t *query,
                           coap_pdu_t *response) {
  assert(device != 0);
  assert(endpoint != 0);
  assert(cluster != 0);
  QCBOREncodeContext cec;
  UsefulBuf_MAKE_STACK_UB(response_buffer,
                          11); // pre-calculated space for entry-point resources
                               // ["a","b","c","n","r"]
  UsefulBufC response_result = NULLUsefulBufC;

  // encode cluster resources as cbor array: a(ttributes), c(ommands),
  // b(indings), r(eport configurations) and n(otifications)
  QCBOREncode_Init(&cec, response_buffer);
  QCBOREncode_OpenArray(&cec);
  QCBOREncode_AddSZString(&cec, "a");
  QCBOREncode_AddSZString(&cec, "b");
  QCBOREncode_AddSZString(&cec, "c");
  QCBOREncode_AddSZString(&cec, "n");
  QCBOREncode_AddSZString(&cec, "r");
  QCBOREncode_CloseArray(&cec);
  QCBORError ceerr = QCBOREncode_Finish(&cec, &response_result);
  assert(ceerr == QCBOR_SUCCESS);

  // respond with 205 Content, media type application/cbor
  response->code = COAP_RESPONSE_CODE(205);
  coap_add_data_blocked_response(resource, session, request, response, token,
                                 COAP_MEDIATYPE_APPLICATION_CBOR, -1,
                                 response_result.len, response_result.ptr);
}

// GET /zcl/e/<eid>/<cl>/a
void dd_handle_attributes_get(dd_device *device, dd_endpoint *endpoint,
                              dd_cluster *cluster,
                              struct coap_resource_t *resource,
                              coap_session_t *session, coap_pdu_t *request,
                              coap_binary_t *token, coap_string_t *query,
                              coap_pdu_t *response) {
  assert(device != 0);
  assert(endpoint != 0);
  assert(cluster != 0);
  QCBOREncodeContext cec;
  UsefulBuf_MAKE_STACK_UB(response_buffer,
                          1024); // TODO: use meaningful estimate
  UsefulBufC response_result = NULLUsefulBufC;

  // evaluate query, if any
  if (query != 0 && query->length > 0) {
    /*
     * in GET only f is supported.
     *
     * allowed filter pattern: f=<item>[,<item>] -
     * item=<aid>,<aid>+n,<aid>-<aid>
     */

    // only support f=, so that **must** be the start of query
    if (query->length < 2 || query->s[0] != 'f' || query->s[1] != '=') {
      printf("query \"%s\" not supported!\n", query->s);
      goto error;
    }

    /*
     * stateful query parser
     * - 0: before parsing new item (number or wildcard)
     * - 1: currently parsing the first number (<aid> or <start>)
     * - 2: before parsing the second number (<count>)
     * - 3: currently parsing the second number (<count>)
     * - 4: before parsing the second number (<end>)
     * - 5: currently parsing the second number (<end>)
     * - 255: error
     */
    uint8_t state = 0;
    size_t i_first = 0, i_second = 0;
    for (size_t i = 2; i <= query->length; i++) {
      switch (query->s[i]) {
      case '*': { // item is a wildcard
        if (state != 0) {
          // unexpected state, parsing failed
          i = query->length;
          state = 255;
        }

        printf("filter item: *\n");
        state = 0;
        break;
      }
      case '0' ... '9':   // Note: 0 is a valid attribute identifier ...
      case 'a' ... 'f': { // start of a number
        switch (state) {
        case 0: // start of first number
          state = 1;
          i_first = i;
          break;
        case 2: // start of second number (<count>)
          state = 3;
          i_second = i;
          break;
        case 4: // start of second number (<end>)
          state = 5;
          i_second = i;
          break;
        default:
          i = query->length;
          state = 255;
          break;
        }
        break;
      }
      case '+': { // item is a count expression (<start>+<count>)
        if (state == 1) {
          state = 2;
        } else {
          // unexpected state, parsing failed
          i = query->length;
          state = 255;
        }
        break;
      }
      case '-': { // item is a range expression (<start>+<end>)
        if (state == 1) {
          state = 4;
        } else {
          // unexpected state, parsing failed
          i = query->length;
          state = 255;
        }
        break;
      }
      case '\0':
      case ',': { // end of item
        switch (state) {
        case 1: { // simple item: <aid>
          uint8_t *aid_end = 0;
          unsigned long int aid =
              strtoul((char *)&query->s[i_first], (char **)&aid_end, 16);
          assert(aid != 0 ||
                 query->s[i_first] ==
                     '0'); // can this ever happen (failure to parse number)?
          if (aid > UINT16_MAX) {
            state = 255;
            break;
          }

          printf("filter item: %lx\n", aid);
          state = 0;
          break;
        }
        case 3: { // count expression <start>+<count>
          uint8_t *start_end = 0;
          unsigned long int start =
              strtoul((char *)&query->s[i_first], (char **)&start_end, 16);
          assert(start != 0 ||
                 query->s[i_first] ==
                     '0'); // can this ever happen (failure to parse number)?
          if (start > UINT16_MAX) {
            state = 255;
            break;
          }

          uint8_t *count_end = 0;
          unsigned long int count =
              strtoul((char *)&query->s[i_second], (char **)&count_end, 16);
          assert(count != 0 ||
                 query->s[i_second] ==
                     '0'); // can this ever happen (failure to parse number)?
          // TODO: accept or reject 0 count?
          if (count > UINT16_MAX) {
            state = 255;
            break;
          }

          printf("filter item: %lx + %lx\n", start, count);
          state = 0;
          break;
        }
        case 5: { // range expression <start>+<count>
          uint8_t *start_end = 0;
          unsigned long int start =
              strtoul((char *)&query->s[i_first], (char **)&start_end, 16);
          assert(start != 0 ||
                 query->s[i_first] ==
                     '0'); // can this ever happen (failure to parse number)?
          if (start > UINT16_MAX) {
            state = 255;
            break;
          }

          uint8_t *end_end = 0;
          unsigned long int end =
              strtoul((char *)&query->s[i_second], (char **)&end_end, 16);
          assert(end != 0 ||
                 query->s[i_second] ==
                     '0'); // can this ever happen (failure to parse number)?
          if (end > UINT16_MAX) {
            state = 255;
            break;
          }

          printf("filter item: %lx - %lx\n", start, end);
          state = 0;
          break;
        }
        default:
          // unexpected state, parsing failed
          i = query->length;
          state = 255;
          break;
        }
        break;
      }
      default: { // unexpected character
        i = query->length;
        state = 255;
        break;
      }
      }
    }
    assert(state == 0 || state == 255);

    if (state == 255) {
      // parsing failed, respond with failure
      printf("parsing query failed!\n");
      goto error;
    }
  }

  if (0) {
    // TODO: filtered result
    goto success;
  } else {
    // encode attribute ids as cbor array
    QCBOREncode_Init(&cec, response_buffer);
    QCBOREncode_OpenArray(&cec);
    for (int i = 0; i < cluster->attributes_length; i++) {
      QCBOREncode_AddUInt64(&cec, cluster->attributes[i]->id);
    }
    QCBOREncode_CloseArray(&cec);
    QCBORError ceerr = QCBOREncode_Finish(&cec, &response_result);
    assert(ceerr == QCBOR_SUCCESS);

    // attach list to response
    coap_add_data_blocked_response(resource, session, request, response, token,
                                   COAP_MEDIATYPE_APPLICATION_CBOR, -1,
                                   response_result.len, response_result.ptr);

    // deliver
    goto success_content;
  }

success:
  // 200 empty return case
  response->code = COAP_RESPONSE_CODE(200);
  return;

success_content:
  // 205 content return case
  response->code = COAP_RESPONSE_CODE(205);
  return;

error:
  // 400 return case
  response->code = COAP_RESPONSE_CODE(400);
  return;
}

// POST /zcl/e/<eid>/<cl>/a
void dd_handle_attributes_post(dd_device *device, dd_endpoint *endpoint,
                               dd_cluster *cluster,
                               struct coap_resource_t *resource,
                               coap_session_t *session, coap_pdu_t *request,
                               coap_binary_t *token, coap_string_t *query,
                               coap_pdu_t *response) {
  printf("attribute mass update not implemented ...\n");
}

// GET /zcl/e/<eid>/<cl>/a/<aid>
void dd_handle_attribute_get(dd_device *device, dd_endpoint *endpoint,
                             dd_cluster *cluster, dd_attribute *attribute,
                             struct coap_resource_t *resource,
                             coap_session_t *session, coap_pdu_t *request,
                             coap_binary_t *token, coap_string_t *query,
                             coap_pdu_t *response) {
  assert(device != 0);
  assert(endpoint != 0);
  assert(cluster != 0);
  assert(attribute != 0);
  QCBOREncodeContext cec;
  UsefulBuf_MAKE_STACK_UB(response_buffer,
                          1024); // TODO: use meaningful estimate
  UsefulBufC response_result = NULLUsefulBufC;

  // encode attribute identifier and value as cbor map
  QCBOREncode_Init(&cec, response_buffer);
  QCBOREncode_OpenMap(&cec);
  char buffer[1024]; // TODO: estimate buffer size QQ
  dd_cbor_add_value_keyn(&cec, attribute->id,
                         attribute->read(buffer, sizeof(buffer)));
  QCBOREncode_CloseMap(&cec);
  QCBORError ceerr = QCBOREncode_Finish(&cec, &response_result);
  assert(ceerr == QCBOR_SUCCESS);

  // respond with 205 Content, media type application/cbor
  response->code = COAP_RESPONSE_CODE(205);
  coap_add_data_blocked_response(resource, session, request, response, token,
                                 COAP_MEDIATYPE_APPLICATION_CBOR, -1,
                                 response_result.len, response_result.ptr);
}

// PUT /zcl/e/<eid>/<cl>/a/<aid>
void dd_handle_attribute_put(dd_device *device, dd_endpoint *endpoint,
                             dd_cluster *cluster, dd_attribute *attribute,
                             struct coap_resource_t *resource,
                             coap_session_t *session, coap_pdu_t *request,
                             coap_binary_t *token, coap_string_t *query,
                             coap_pdu_t *response) {
  assert(device != 0);
  assert(endpoint != 0);
  assert(cluster != 0);
  assert(attribute != 0);
  QCBORDecodeContext cdc;
  UsefulBufC request_buffer;
  QCBORItem item;
  QCBORError cderr;

  // parse payload
  if (coap_get_data(request, &request_buffer.len,
                    (uint8_t **)&request_buffer.ptr) != 1) {
    // TODO: zcl status code
    fprintf(stderr, "no payload\n");
    goto dd_handle_attribute_put__400;
  }
  // TODO: validate encoding declaration (coap)
  QCBORDecode_Init(&cdc, request_buffer, QCBOR_DECODE_MODE_NORMAL);

  // expect a map: attribute id -> value
  cderr = QCBORDecode_GetNext(&cdc, &item);
  if (cderr != QCBOR_SUCCESS || item.uDataType != QCBOR_TYPE_MAP ||
      item.val.uCount != 1) {
    // TODO: zcl status code
    fprintf(stderr, "not map with 1 item, instead type=%i, count=%u\n",
            item.uDataType, item.val.uCount);
    goto dd_handle_attribute_put__400;
  }

  // get key-value pair aid -> value
  cderr = QCBORDecode_GetNext(&cdc, &item);
  if (cderr != QCBOR_SUCCESS ||
      item.uLabelType !=
          QCBOR_TYPE_INT64 /* everything <= int64_max reports int64 */) {
    // TODO: zcl status code
    fprintf(stderr, "label not int\n");
    goto dd_handle_attribute_put__400;
  }
  int64_t aid = item.label.int64;
  if (0 > aid || aid > UINT16_MAX) {
    // TODO: zcl status code
    fprintf(stderr, "aid out of range\n");
    goto dd_handle_attribute_put__400;
  }
  char buffer[1024]; // TODO: better size prediction
  dd_value *value = dd_cbor_get_value(&item, buffer, sizeof(buffer));
  if (value == 0) {
    fprintf(stderr, "couldn't parse value\n");
    // TODO: zcl status code
    goto dd_handle_attribute_put__400;
  }

  attribute->write(value);
  // TODO: data type mismatch possible, handle as 400 ...
  goto dd_handle_attribute_put__204;

dd_handle_attribute_put__204:
  response->code = COAP_RESPONSE_CODE(204);
  return;

dd_handle_attribute_put__400:
  response->code = COAP_RESPONSE_CODE(400);
  return;
}

// GET /zcl/e/<eid>/<cl>/b
void dd_handle_bindings_get(dd_device *device, dd_endpoint *endpoint,
                            dd_cluster *cluster,
                            struct coap_resource_t *resource,
                            coap_session_t *session, coap_pdu_t *request,
                            coap_binary_t *token, coap_string_t *query,
                            coap_pdu_t *response) {
  assert(device != 0);
  assert(endpoint != 0);
  assert(cluster != 0);
  QCBOREncodeContext cec;
  UsefulBuf_MAKE_STACK_UB(response_buffer,
                          1024); // TODO: use meaningful estimate
  UsefulBufC response_result = NULLUsefulBufC;

  // encode binding ids as cbor array
  QCBOREncode_Init(&cec, response_buffer);
  QCBOREncode_OpenArray(&cec);
  for (int i = 0; i < cluster->bindings_length; i++) {
    QCBOREncode_AddUInt64(&cec, cluster->bindings[i]->id);
  }
  QCBOREncode_CloseArray(&cec);
  QCBORError ceerr = QCBOREncode_Finish(&cec, &response_result);
  assert(ceerr == QCBOR_SUCCESS);

  // respond with 205 Content, media type application/cbor
  response->code = COAP_RESPONSE_CODE(205);
  coap_add_data_blocked_response(resource, session, request, response, token,
                                 COAP_MEDIATYPE_APPLICATION_CBOR, -1,
                                 response_result.len, response_result.ptr);
}

static void dd_handle_bindings__parse_entry(QCBORDecodeContext *ctx,
                                            uint8_t *rid, dd_uri **uri,
                                            void *buffer, size_t buffer_size) {
  assert(ctx != 0);
  assert(uri != 0);
  assert(rid != 0);
  assert(buffer != 0);
  QCBORError cderr;
  QCBORItem item;

  // expect map with report id and uri: r -> <rid> [, u -> <uri>]
  cderr = QCBORDecode_GetNext(ctx, &item);
  if (cderr != QCBOR_SUCCESS || item.uDataType != QCBOR_TYPE_MAP ||
      item.val.uCount == 0 || item.val.uCount > 2) {
    return;
  }
  int64_t _rid = 0;
  dd_uri *_uri = 0;
  for (int i = 0; i < 2; i++) {
    cderr = QCBORDecode_GetNext(ctx, &item);
    // expect "r" or "u" string keys
    if (item.uLabelType != QCBOR_TYPE_TEXT_STRING ||
        item.label.string.len != 1) {
      return;
    }
    if (((const char *)item.label.string.ptr)[0] == 'r') {
      if (item.uDataType !=
              QCBOR_TYPE_INT64 /* everything <= int64_max reports int64 */
          || 0 > item.val.int64 || item.val.int64 > UINT8_MAX) {
        return;
      }
      _rid = item.val.int64;
    } else if (((const char *)item.label.string.ptr)[0] == 'u') {
      _uri = dd_cbor_get_uri(&item, buffer, buffer_size);
      if (_uri == 0) {
        return;
      }
      // TODO: verify uri???
    } else {
      return;
    }
  }

  *uri = _uri;
  *rid = _rid;
}

// POST /zcl/e/<eid>/<cl>/b
void dd_handle_bindings_post(dd_device *device, dd_endpoint *endpoint,
                             dd_cluster *cluster,
                             struct coap_resource_t *resource,
                             coap_session_t *session, coap_pdu_t *request,
                             coap_binary_t *token, coap_string_t *query,
                             coap_pdu_t *response) {
  assert(device != 0);
  assert(endpoint != 0);
  assert(cluster != 0);
  QCBORDecodeContext cdc;
  UsefulBufC request_buffer;

  // parse payload
  if (coap_get_data(request, &request_buffer.len,
                    (uint8_t **)&request_buffer.ptr) != 1) {
    // TODO: zcl status code
    goto dd_handle_bindings_post__400;
  }
  // TODO: validate encoding declaration (coap)
  QCBORDecode_Init(&cdc, request_buffer, QCBOR_DECODE_MODE_NORMAL);
  uint8_t rid = 0;
  dd_uri *uri = 0;
  char uri_buffer[100]; // TODO: use meaningful estimate
  dd_handle_bindings__parse_entry(&cdc, &rid, &uri, (void *)uri_buffer,
                                  sizeof(uri_buffer));
  // QCBORDecode_Finish(&cdc); // TODO??

  if (uri == 0) {
    // uri is required
    // TODO: zcl status code
    goto dd_handle_bindings_post__400;
  }

  // look-up report configuration
  dd_report *report = 0;
  if (rid != 0) {
    for (size_t i = 0; i < cluster->reports_length; i++) {
      if (cluster->reports[i] != 0 && cluster->reports[i]->id == rid) {
        report = cluster->reports[i];
      }
    }
    if (report == 0) {
      // report id does not exist
      // TODO: zcl status code
      goto dd_handle_bindings_post__400;
    }
  }

  // create candidate binding entry
  char buffer[1024];
  dd_binding *candidate = (void *)buffer;
  bzero(candidate, sizeof(dd_binding));
  dd_copy_uri(candidate->_buffer, sizeof(buffer) - sizeof(dd_binding), uri);
  candidate->uri = (void *)candidate->_buffer;
  candidate->rid = rid;
  candidate->length = sizeof(dd_uri) + uri->length;

  // TODO: find duplicate entry in bindings table ...

  // check capacity of cluster instance binding table
  if (cluster->bindings_length >= DD_CLUSTER_BINDINGS_MAX) {
    // oom
    response->code = COAP_RESPONSE_CODE(500);
    return;
  }

  // insert to storage
  dd_binding *binding =
      dd_storage_bindings_put(endpoint->id, cluster->id, candidate);
  if (binding == 0) {
    // storage full
    goto dd_handle_bindings_post__500;
  }
  cluster->bindings[cluster->bindings_length++] = binding;

  // return success + uri of new binding
  {
    const char *format = "/zcl/e/%x/%c%x/b/%x";
    int len = snprintf(0, 0, format, endpoint->id, cluster->role, cluster->id,
                       binding->id);
    void *location = coap_add_option_later(response, COAP_OPTION_LOCATION_PATH,
                                           len); // TODO: this can fail?!!!
    snprintf(location, len, format, cluster->role, cluster->id,
             !report ? 0 : report->id);
  }
  goto dd_handle_bindings_post__201;

dd_handle_bindings_post__201:
  response->code = COAP_RESPONSE_CODE(201);
  return;

dd_handle_bindings_post__400:
  response->code = COAP_RESPONSE_CODE(400);
  return;

dd_handle_bindings_post__500:
  response->code = COAP_RESPONSE_CODE(500);
  return;
}

// GET /zcl/e/<eid>/<cl>/<bid>
void dd_handle_binding_get(dd_device *device, dd_endpoint *endpoint,
                           dd_cluster *cluster, dd_binding *binding,
                           struct coap_resource_t *resource,
                           coap_session_t *session, coap_pdu_t *request,
                           coap_binary_t *token, coap_string_t *query,
                           coap_pdu_t *response) {
  assert(device != 0);
  assert(endpoint != 0);
  assert(cluster != 0);
  assert(binding != 0);
  QCBOREncodeContext cec;
  UsefulBuf_MAKE_STACK_UB(response_buffer,
                          1024); // TODO: estimate meaningful size
  UsefulBufC response_result = NULLUsefulBufC;

  // encode binding entry as cbor map
  QCBOREncode_Init(&cec, response_buffer);
  QCBOREncode_OpenMap(&cec);
  dd_cbor_add_uri_key(&cec, "u", binding->uri);
  QCBOREncode_AddUInt64ToMap(&cec, "r", binding->rid);
  QCBOREncode_CloseMap(&cec);
  QCBORError ceerr = QCBOREncode_Finish(&cec, &response_result);
  assert(ceerr == QCBOR_SUCCESS);

  // respond with 205 Content, media type application/cbor
  response->code = COAP_RESPONSE_CODE(205);
  coap_add_data_blocked_response(resource, session, request, response, token,
                                 COAP_MEDIATYPE_APPLICATION_CBOR, -1,
                                 response_result.len, response_result.ptr);
}

// PUT /zcl/e/<eid>/<cl>/<bid>
void dd_handle_binding_put(dd_device *device, dd_endpoint *endpoint,
                           dd_cluster *cluster, dd_binding *binding,
                           struct coap_resource_t *resource,
                           coap_session_t *session, coap_pdu_t *request,
                           coap_binary_t *token, coap_string_t *query,
                           coap_pdu_t *response) {
  assert(device != 0);
  assert(endpoint != 0);
  assert(cluster != 0);
  assert(binding != 0);
  QCBORDecodeContext cdc;
  UsefulBufC request_buffer;
  char candidate_buffer[1024]; // TODO: make meaningful estimate
  dd_binding *candidate = (void *)candidate_buffer;
  bzero(candidate, sizeof(dd_binding));
  candidate->id = binding->id;

  // parse payload
  if (coap_get_data(request, &request_buffer.len,
                    (uint8_t **)&request_buffer.ptr) != 1) {
    // TODO: zcl status code
    goto dd_handle_binding_put__400;
  }
  // TODO: validate encoding declaration (coap)
  QCBORDecode_Init(&cdc, request_buffer, QCBOR_DECODE_MODE_NORMAL);
  dd_handle_bindings__parse_entry(
      &cdc, &candidate->rid, &candidate->uri, candidate->_buffer,
      sizeof(candidate_buffer) - sizeof(dd_binding));
  // QCBORDecode_Finish(&cdc); // TODO??

  if (candidate->uri == 0) {
    // uri is required
    // TODO: zcl status code
    goto dd_handle_binding_put__400;
  }
  candidate->length += candidate->uri->length;

  // look-up report configuration
  dd_report *report = 0;
  if (candidate->rid != 0) {
    for (size_t i = 0; i < cluster->reports_length; i++) {
      if (cluster->reports[i] != 0 &&
          cluster->reports[i]->id == candidate->rid) {
        report = cluster->reports[i];
      }
    }
    if (report == 0) {
      // report id does not exist
      // TODO: zcl status code
      goto dd_handle_binding_put__400;
    }
  }

  // check for duplicate
  for (size_t i = 0; i < cluster->bindings_length; i++) {
    dd_binding *other = cluster->bindings[i];
    if (other->rid == candidate->rid &&
        candidate->uri->scheme == other->uri->scheme &&
        strcmp(candidate->uri->host, other->uri->host) == 0 &&
        candidate->uri->port == other->uri->port &&
        strcmp(candidate->uri->path, other->uri->path) == 0) {
      // duplicate detected
      // TODO: ZCL status code DUPLICATE_EXISTS
      goto dd_handle_binding_put__400;
    }
  }

  // update binding
  dd_storage_bindings_update(binding, candidate);
  // TODO: also update pointer in resource tree ... ?

  // done
  goto dd_handle_binding_put__204;

dd_handle_binding_put__204:
  response->code = COAP_RESPONSE_CODE(204);
  return;

dd_handle_binding_put__400:
  response->code = COAP_RESPONSE_CODE(400);
  return;
}

// DELETE /zcl/e/<eid>/<cl>/<bid>
void dd_handle_binding_delete(dd_device *device, dd_endpoint *endpoint,
                              dd_cluster *cluster, dd_binding *binding,
                              struct coap_resource_t *resource,
                              coap_session_t *session, coap_pdu_t *request,
                              coap_binary_t *token, coap_string_t *query,
                              coap_pdu_t *response) {
  assert(device != 0);
  assert(endpoint != 0);
  assert(cluster != 0);
  assert(binding != 0);

  // delete from resource tree
  int deleted = 0;
  for (size_t i = 0; i < cluster->bindings_length; i++) {
    if (cluster->bindings[i]->id == binding->id) {
      while (++i < cluster->bindings_length) {
        cluster->bindings[i - 1] = cluster->bindings[i];
      }
      cluster->bindings_length--;
      cluster->bindings[cluster->bindings_length] = 0;
      deleted = 1;
    }
  }
  assert(deleted == 1);

  // delete from storage
  dd_storage_bindings_delete(binding);

  // done
  response->code = COAP_RESPONSE_CODE(202);
}

// GET /zcl/e/<eid>/<cl>/c
void dd_handle_commands_get(dd_device *device, dd_endpoint *endpoint,
                            dd_cluster *cluster,
                            struct coap_resource_t *resource,
                            coap_session_t *session, coap_pdu_t *request,
                            coap_binary_t *token, coap_string_t *query,
                            coap_pdu_t *response) {
  assert(device != 0);
  assert(endpoint != 0);
  assert(cluster != 0);
  QCBOREncodeContext cec;
  UsefulBuf_MAKE_STACK_UB(response_buffer,
                          1024); // TODO: estimate meaningful size
  UsefulBufC response_result = NULLUsefulBufC;

  // encode commands as cbor array
  QCBOREncode_Init(&cec, response_buffer);
  QCBOREncode_OpenArray(&cec);
  for (int i = 0; i < cluster->commands_length; i++) {
    QCBOREncode_AddUInt64(&cec, cluster->commands[i]->id);
  }
  QCBOREncode_CloseArray(&cec);
  QCBORError ceerr = QCBOREncode_Finish(&cec, &response_result);
  assert(ceerr == QCBOR_SUCCESS);

  // respond with 205 Content, media type application/cbor
  response->code = COAP_RESPONSE_CODE(205);
  coap_add_data_blocked_response(resource, session, request, response, token,
                                 COAP_MEDIATYPE_APPLICATION_CBOR, -1,
                                 response_result.len, response_result.ptr);
}

// GET /zcl/e/<eid>/<cl>/c/<cid>
void dd_handle_command_post(dd_device *device, dd_endpoint *endpoint,
                            dd_cluster *cluster, dd_command *command,
                            struct coap_resource_t *resource,
                            coap_session_t *session, coap_pdu_t *request,
                            coap_binary_t *token, coap_string_t *query,
                            coap_pdu_t *response) {
  assert(device != 0);
  assert(endpoint != 0);
  assert(cluster != 0);
  assert(command != 0);

  // TODO: figure out how to handle arguments
  command->exec();

  // payload or not - code is 204 ...
  response->code = COAP_RESPONSE_CODE(204);
}

// POST /zcl/e/<eid>/<cl>/n
void dd_handle_notification_post(dd_device *device, dd_endpoint *endpoint,
                                 dd_cluster *cluster,
                                 struct coap_resource_t *resource,
                                 coap_session_t *session, coap_pdu_t *request,
                                 coap_binary_t *token, coap_string_t *query,
                                 coap_pdu_t *response) {
  assert(device != 0);
  assert(endpoint != 0);
  assert(cluster != 0);
  char notification_buffer[1024];
  UsefulBufC request_buffer;
  QCBORDecodeContext cdc;
  QCBORItem item;
  QCBORError cderr;

  // parse payload
  if (coap_get_data(request, &request_buffer.len,
                    (uint8_t **)&request_buffer.ptr) != 1) {
    // TODO: zcl status code
    goto dd_handle_notification_post__400;
  }
  // TODO: validate encoding declaration (coap)
  QCBORDecode_Init(&cdc, request_buffer, QCBOR_DECODE_MODE_NORMAL);

  dd_notification notification = {0};
  cderr = QCBORDecode_GetNext(&cdc, &item);
  if (cderr != QCBOR_SUCCESS || item.uDataType != QCBOR_TYPE_MAP ||
      item.val.uCount == 0 || item.val.uCount > 5) {
    // expect map with notification source and attribute values "u", "r", "b",
    // "t", "a"
    goto dd_handle_notification_post__400;
  }
  uint16_t nitems = item.val.uCount;
  for (uint16_t i = 0; i < nitems; i++) {
    cderr = QCBORDecode_GetNext(&cdc, &item);
    if (cderr != QCBOR_SUCCESS || item.uLabelType != QCBOR_TYPE_TEXT_STRING ||
        item.label.string.len != 1) {
      // expect string key length 1
      goto dd_handle_notification_post__400;
    }
    switch (((const char *)item.label.string.ptr)[0]) {
    case 'a': {
      if (item.uDataType != QCBOR_TYPE_MAP) {
        // expect map with attributes id => value pairs
        goto dd_handle_notification_post__400;
      }
      uint16_t nattributes = item.val.uCount;
      for (uint16_t j = 0; j < nattributes; j++) {
        cderr = QCBORDecode_GetNext(&cdc, &item);
        if (cderr != QCBOR_SUCCESS ||
            item.uLabelType !=
                QCBOR_TYPE_INT64 /* everything <= int64_max reports int64 */
            || 0 > item.label.int64 || item.label.int64 > UINT16_MAX) {
          // expect attribute id (uint16) key
          goto dd_handle_notification_post__400;
        }
        // TODO: extract value
      }
      break;
    }
    case 'b':
      if (item.uDataType !=
              QCBOR_TYPE_INT64 /* everything <= int64_max reports int64 */
          || 0 > item.val.int64 || item.val.int64 > UINT8_MAX) {
        // expect uint8
        goto dd_handle_notification_post__400;
      }
      notification.bid = item.val.int64;
      break;
    case 'r':
      if (item.uDataType !=
              QCBOR_TYPE_INT64 /* everything <= int64_max reports int64 */
          || 0 > item.val.int64 || item.val.int64 > UINT8_MAX) {
        // expect uint8
        goto dd_handle_notification_post__400;
      }
      notification.rid = item.val.int64;
      break;
    case 't':
      if (item.uDataType != QCBOR_TYPE_DATE_EPOCH) {
        // expect unixtime
        goto dd_handle_notification_post__400;
      }
      notification.timestamp = item.val.int64;
      break;
    case 'u':
      notification.uri = dd_cbor_get_uri(&item, notification_buffer,
                                         sizeof(notification_buffer));
      if (notification.uri == 0) {
        // parsing failed
        goto dd_handle_notification_post__400;
      }
      break;
    default:
      goto dd_handle_notification_post__400;
    }
  }

  // TODO: check if all required fields were provided

  // call user notification handler
  if (cluster->notify == 0) {
    // cluster does not support notifications
    // TODO: what is correct response code?
    goto dd_handle_notification_post__400;
  }
  cluster->notify(&notification);

  goto dd_handle_notification_post__204;

dd_handle_notification_post__204:
  response->code = COAP_RESPONSE_CODE(204);
  return;

dd_handle_notification_post__400:
  response->code = COAP_RESPONSE_CODE(400);
  return;
}

// GET /zcl/e/<eid>/<cl>/r
void dd_handle_reports_get(dd_device *device, dd_endpoint *endpoint,
                           dd_cluster *cluster,
                           struct coap_resource_t *resource,
                           coap_session_t *session, coap_pdu_t *request,
                           coap_binary_t *token, coap_string_t *query,
                           coap_pdu_t *response) {
  assert(device != 0);
  assert(endpoint != 0);
  assert(cluster != 0);
  QCBOREncodeContext cec;
  UsefulBuf_MAKE_STACK_UB(response_buffer,
                          1024); // TODO: estimate meaningful size
  UsefulBufC response_result = NULLUsefulBufC;

  // encode report identifiers as cbor array
  QCBOREncode_Init(&cec, response_buffer);
  QCBOREncode_OpenArray(&cec);
  for (size_t i = 0; i < cluster->reports_length; i++) {
    QCBOREncode_AddUInt64(&cec, cluster->reports[i]->id);
  }
  QCBOREncode_CloseArray(&cec);
  QCBORError ceerr = QCBOREncode_Finish(&cec, &response_result);
  assert(ceerr == QCBOR_SUCCESS);

  // respond with 205 Content, media type application/cbor
  response->code = COAP_RESPONSE_CODE(205);
  coap_add_data_blocked_response(resource, session, request, response, token,
                                 COAP_MEDIATYPE_APPLICATION_CBOR, -1,
                                 response_result.len, response_result.ptr);
}

static void dd_handle_reports__parse_entry(QCBORDecodeContext *ctx,
                                           dd_report **report,
                                           void *report_buffer,
                                           size_t report_buffer_size,
                                           dd_uri **uri, void *uri_buffer,
                                           size_t uri_buffer_size) {
  assert(ctx != 0);
  assert(report != 0);
  assert(report_buffer != 0);
  assert(uri != 0);
  assert(uri_buffer != 0);
  QCBORError cderr;
  QCBORItem item;

  // expect map with "n", "x", "a" [, "u"] entries
  cderr = QCBORDecode_GetNext(ctx, &item);
  if (cderr != QCBOR_SUCCESS || item.uDataType != QCBOR_TYPE_MAP ||
      item.val.uCount == 0 || item.val.uCount > 4) {
    return;
  }
  uint16_t nentries = item.val.uCount;
  assert(report_buffer_size >= sizeof(dd_report));
  dd_report *_report = report_buffer;
  bzero(_report, sizeof(dd_report));
  dd_uri *_uri = 0;
  int fields_parsed = 0;
  for (int i = 0; i < nentries; i++) {
    cderr = QCBORDecode_GetNext(ctx, &item);
    // expect string key length 1
    if (item.uLabelType != QCBOR_TYPE_TEXT_STRING ||
        item.label.string.len != 1) {
      return;
    } else if (((const char *)item.label.string.ptr)[0] == 'a') {
      if (item.uDataType != QCBOR_TYPE_MAP) {
        // expect map
        return;
      }
      // parse all attribute configurations
      _report->attributes = dd_cbor_get_report_attribute_configurations(
          ctx, item.val.uCount, _report->_buffer,
          report_buffer_size - sizeof(dd_report), &_report->attributes_length);
      if (_report->attributes == 0) {
        // parsing failed - or 0 attribute configurations ...
        return;
      }
      _report->length += _report->attributes_length;
      fields_parsed |= 1;
    } else if (((const char *)item.label.string.ptr)[0] == 'n') {
      if (item.uDataType !=
              QCBOR_TYPE_INT64 /* everything <= int64_max reports int64 */
          || 0 > item.val.int64 || item.val.int64 > UINT16_MAX) {
        // expect uint16
        return;
      }
      _report->min_reporting_interval = item.val.int64;
      fields_parsed |= 2;
    } else if (((const char *)item.label.string.ptr)[0] == 'u') {
      _uri = dd_cbor_get_uri(&item, uri_buffer, uri_buffer_size);
      if (_uri == 0) {
        // parsing failed
        return;
      }
      fields_parsed |= 4;
    } else if (((const char *)item.label.string.ptr)[0] == 'x') {
      if (item.uDataType !=
              QCBOR_TYPE_INT64 /* everything <= int64_max reports int64 */
          || 0 > item.val.int64 || item.val.int64 > UINT16_MAX) {
        // expect uint16
        return;
      }
      _report->max_reporting_interval = item.val.int64;
      fields_parsed |= 8;
    } else {
      // unexpected key
      return;
    }
  }
  if ((fields_parsed & 0b1011) != 0b1011) {
    // any of a,n,x missing :(
    return;
  }

  *uri = _uri;
  *report = _report;
}

// POST /zcl/e/<eid>/<cl>/r
void dd_handle_reports_post(dd_device *device, dd_endpoint *endpoint,
                            dd_cluster *cluster,
                            struct coap_resource_t *resource,
                            coap_session_t *session, coap_pdu_t *request,
                            coap_binary_t *token, coap_string_t *query,
                            coap_pdu_t *response) {
  assert(device != 0);
  assert(endpoint != 0);
  assert(cluster != 0);
  QCBORDecodeContext cdc;
  UsefulBufC request_buffer;

  // parse payload
  if (coap_get_data(request, &request_buffer.len,
                    (uint8_t **)&request_buffer.ptr) != 1) {
    // TODO: zcl status code
    goto dd_handle_reports_post__400;
  }
  // TODO: validate encoding declaration (coap)
  QCBORDecode_Init(&cdc, request_buffer, QCBOR_DECODE_MODE_NORMAL);
  dd_report *report = 0;
  char report_buffer[1024]; // TODO: use meaningful estimate
  dd_uri *uri;
  char uri_buffer[1024]; // TODO: use meaningful estimate
  dd_handle_reports__parse_entry(&cdc, &report, report_buffer,
                                 sizeof(report_buffer), &uri, uri_buffer,
                                 sizeof(uri_buffer));
  if (report == 0) {
    // parsing failed
    goto dd_handle_reports_post__400;
  }
  // QCBORDecode_Finish(&cdc); // TODO??

  // Note: presence of uri means that a binding entry should be created!
  // TODO: verify attributes exist, and thresholds are of the same data type -->
  // collect errors for verbose response

  // check capacity of cluster report table
  if (cluster->reports_length >= DD_CLUSTER_REPORTS_MAX) {
    goto dd_handle_reports_post__500;
  }
  // TODO: create binding entry from uri and report id

  // write to persistent storage
  report = dd_storage_reports_put(endpoint->id, cluster->id, report);
  if (report == 0) {
    // storage full
    goto dd_handle_reports_post__500;
  }
  cluster->reports[cluster->reports_length++] = report;

  // return created + new resource uri "/zcl/e/<eid>/<cl>/r/<rid>"
  {
    const char *format = "/zcl/e/%x/%c%x/r/%x";
    int len = snprintf(0, 0, format, endpoint->id, cluster->role, cluster->id,
                       report->id);
    void *location = coap_add_option_later(response, COAP_OPTION_LOCATION_PATH,
                                           len); // TODO: this can fail?!!!
    snprintf(location, len, format, cluster->role, cluster->id, report->id);
  }
  goto dd_handle_reports_post__201;

dd_handle_reports_post__201:
  response->code = COAP_RESPONSE_CODE(201);
  return;

dd_handle_reports_post__400:
  response->code = COAP_RESPONSE_CODE(400);
  return;

dd_handle_reports_post__500:
  response->code = COAP_RESPONSE_CODE(500);
  return;
}

// GET /zcl/e/<eid>/<cl>/r/<rid>
void dd_handle_report_get(dd_device *device, dd_endpoint *endpoint,
                          dd_cluster *cluster, dd_report *report,
                          struct coap_resource_t *resource,
                          coap_session_t *session, coap_pdu_t *request,
                          coap_binary_t *token, coap_string_t *query,
                          coap_pdu_t *response) {
  assert(device != 0);
  assert(endpoint != 0);
  assert(cluster != 0);
  assert(report != 0);
  QCBOREncodeContext cec;
  UsefulBuf_MAKE_STACK_UB(response_buffer,
                          1024); // TODO: estimate meaningful size
  UsefulBufC response_result = NULLUsefulBufC;

  // encode report configuration as cbor map
  QCBOREncode_Init(&cec, response_buffer);
  QCBOREncode_OpenMap(&cec);
  QCBOREncode_OpenMapInMap(&cec, "a");
  for (dd_report_attribute *attribute_configuration = report->attributes;
       attribute_configuration < report->attributes + report->attributes_length;
       attribute_configuration +=
       sizeof(dd_report_attribute) + attribute_configuration->length) {
    QCBOREncode_OpenMapInMapN(&cec, attribute_configuration->aid);
    if (attribute_configuration->high_threshold != 0) {
      dd_cbor_add_value_key(&cec, "h", attribute_configuration->high_threshold);
    }
    if (attribute_configuration->low_threshold != 0) {
      dd_cbor_add_value_key(&cec, "l", attribute_configuration->low_threshold);
    }
    if (attribute_configuration->reportable_change != 0) {
      dd_cbor_add_value_key(&cec, "r",
                            attribute_configuration->reportable_change);
    }
    QCBOREncode_CloseMap(&cec);
  }
  QCBOREncode_CloseMap(&cec);
  QCBOREncode_AddUInt64ToMap(&cec, "n", report->min_reporting_interval);
  QCBOREncode_AddUInt64ToMap(&cec, "x", report->max_reporting_interval);
  QCBOREncode_CloseMap(&cec);
  QCBORError ceerr = QCBOREncode_Finish(&cec, &response_result);
  assert(ceerr == QCBOR_SUCCESS);

  // respond with 205 Content, media type application/cbor
  response->code = COAP_RESPONSE_CODE(205);
  coap_add_data_blocked_response(resource, session, request, response, token,
                                 COAP_MEDIATYPE_APPLICATION_CBOR, -1,
                                 response_result.len, response_result.ptr);
}

// PUT /zcl/e/<eid>/<cl>/r/<rid>
void dd_handle_report_put(dd_device *device, dd_endpoint *endpoint,
                          dd_cluster *cluster, dd_report *report,
                          struct coap_resource_t *resource,
                          coap_session_t *session, coap_pdu_t *request,
                          coap_binary_t *token, coap_string_t *query,
                          coap_pdu_t *response) {
  assert(device != 0);
  assert(endpoint != 0);
  assert(cluster != 0);
  assert(report != 0);

  printf("TODO: update report configuration\n");
}

// DELETE /zcl/e/<eid>/<cl>/r/<rid>
void dd_handle_report_delete(dd_device *device, dd_endpoint *endpoint,
                             dd_cluster *cluster, dd_report *report,
                             struct coap_resource_t *resource,
                             coap_session_t *session, coap_pdu_t *request,
                             coap_binary_t *token, coap_string_t *query,
                             coap_pdu_t *response) {
  assert(device != 0);
  assert(endpoint != 0);
  assert(cluster != 0);
  assert(report != 0);

  // delete from resource tree
  int deleted = 0;
  for (size_t i = 0; i < cluster->reports_length; i++) {
    if (cluster->reports[i]->id == report->id) {
      while (++i < cluster->reports_length) {
        cluster->reports[i - 1] = cluster->reports[i];
      }
      cluster->reports_length--;
      cluster->reports[cluster->reports_length] = 0;
      deleted = 1;
    }
  }
  assert(deleted == 1);

  // update referencing bindings to null report configuration
  for (size_t i = 0; i < cluster->bindings_length; i++) {
    if (cluster->bindings[i]->rid == report->id) {
      cluster->bindings[i]->rid = 0;
      dd_storage_bindings_update(cluster->bindings[i], cluster->bindings[i]);
    }
  }

  // delete from storage
  dd_storage_reports_delete(report);

  // done
  response->code = COAP_RESPONSE_CODE(202);
}

/*
 * Periodic Jobs
 */
static void dd_make_notification(QCBOREncodeContext *ctx, dd_endpoint *endpoint,
                                 dd_cluster *cluster, dd_binding *binding,
                                 dd_report *report, int force) {
  assert(cluster != 0);
  assert(binding != 0);
  assert(report != 0);
  time_t now = time(0);

  // encode notification as cbor map
  QCBOREncode_OpenMap(ctx);
  QCBOREncode_OpenMapInMap(ctx, "a");
  for (dd_report_attribute *attribute_configuration = report->attributes;
       attribute_configuration < report->attributes + report->attributes_length;
       attribute_configuration +=
       sizeof(dd_report_attribute) + attribute_configuration->length) {
    // TODO: direct link from configuration to attribute instance
    dd_attribute *attribute = 0;
    for (size_t j = 0; j < cluster->attributes_length; j++) {
      if (cluster->attributes[j]->id == attribute_configuration->aid) {
        attribute = cluster->attributes[j];
        break;
      }
    }
    assert(attribute != 0);

    // TODO: conditional based on change, min and max ...
    char buffer[1024]; // TODO: use meaningful estimate
    dd_cbor_add_value_keyn(ctx, attribute->id,
                           attribute->read(buffer, sizeof(buffer)));
  }
  QCBOREncode_CloseMap(ctx);
  QCBOREncode_AddUInt64ToMap(ctx, "b", binding->id);
  QCBOREncode_AddUInt64ToMap(ctx, "r", report->id);
  QCBOREncode_AddDateEpochToMap(ctx, "t", now);
  { // build sender uri
    char sender_uri_buffer[1024];
    dd_uri *sender_uri = (void *)sender_uri_buffer;
    bzero(sender_uri, sizeof(dd_uri));
    sender_uri->host = sender_uri->_buffer + sender_uri->length;
    memcpy((void *)sender_uri->host, "localhost", sizeof("localhost"));
    sender_uri->length += sizeof("localhost"); // TODO: real hostname
    sender_uri->path = sender_uri->_buffer + sender_uri->length;
    sender_uri->length += snprintf(
        (void *)sender_uri->path,
        sizeof(sender_uri_buffer) - sizeof(dd_uri) - sender_uri->length,
        "/zcl/e/%x/%c%x", endpoint->id, cluster->role, cluster->id);
    dd_cbor_add_uri_key(ctx, "u", sender_uri);
  }
  QCBOREncode_CloseMap(ctx);
}

int32_t dd_process_bindings(coap_context_t *context, dd_device *device) {
  assert(device != 0);
  uint16_t maysleep = UINT16_MAX;
  time_t now = time(0);
  QCBOREncodeContext cec;
  UsefulBuf_MAKE_STACK_UB(request_buffer,
                          1024); // TODO: estimate meaningful size
  UsefulBufC request_result = NULLUsefulBufC;

  for (int i = 0; i < device->endpoints_length; i++) {
    dd_endpoint *endpoint = device->endpoints[i];
    for (int j = 0; j < endpoint->cluster_length; j++) {
      dd_cluster *cluster = endpoint->cluster[j];
      for (int k = 0; k < cluster->bindings_length; k++) {
        dd_binding *binding = cluster->bindings[k];
        // TODO: direct link from binding to report ...
        dd_report *report = 0;
        for (int l = 0; l < cluster->reports_length; l++) {
          report = cluster->reports[l];
          if (report->id == binding->rid) {
            break;
          }
        }
        if (binding->rid == 0) {
          // TODO: support a default report configuration
          fprintf(stderr, "TODO: support default report configuration\n");
          continue;
        }
        assert(report != 0 && report->id == binding->rid);

        // calculate time report is due
        double elapsed = difftime(now, binding->timestamp);
        uint16_t slack =
            report->max_reporting_interval - report->min_reporting_interval;

        // TODO: pick random time within min and max
        if (elapsed >= report->min_reporting_interval) {
          printf("sending report by time\n");
          // build notification
          QCBOREncode_Init(&cec, request_buffer);
          dd_make_notification(&cec, endpoint, cluster, binding, report, 1);
          QCBORError ceerr = QCBOREncode_Finish(&cec, &request_result);
          assert(ceerr == QCBOR_SUCCESS);

          // create client session
          coap_address_t addr;
          dd_coap_resolve(&addr, binding->uri->host, binding->uri->port);
          coap_session_t *session =
              coap_new_client_session(context, 0, &addr, COAP_PROTO_UDP);
          if (session == 0) {
            fprintf(stderr, "failed to create client session!\n");
            continue;
          }

          // send
          coap_pdu_t *notification = coap_pdu_init(
              COAP_MESSAGE_NON, COAP_REQUEST_POST, coap_new_message_id(session),
              coap_session_max_pdu_size(session));
          if (notification == 0) {
            fprintf(stderr, "Failed to create new coap message!\n");
            continue;
          }
          uint8_t optbuffer[4];
          coap_add_option(notification, COAP_OPTION_URI_PATH,
                          strlen(binding->uri->path) + 1,
                          (const uint8_t *)binding->uri->path);
          coap_add_option(notification, COAP_OPTION_CONTENT_TYPE,
                          coap_encode_var_safe(optbuffer, sizeof(optbuffer),
                                               COAP_MEDIATYPE_APPLICATION_CBOR),
                          optbuffer);
          coap_add_option(notification, COAP_OPTION_SIZE1,
                          coap_encode_var_safe(optbuffer, sizeof(optbuffer),
                                               request_result.len),
                          optbuffer);
          coap_add_data(notification, request_result.len, request_result.ptr);
          // TODO: many magic numbers here, document ...

          coap_send(session, notification);
          coap_session_release(session);

          // remember
          binding->timestamp = now;
          elapsed = 0.0f;
          dd_storage_bindings_update(binding, binding);
          // TODO: also update pointer in resource tree ... ?
        }

        // note time till due next
        if (report->min_reporting_interval - elapsed < maysleep)
          maysleep = report->min_reporting_interval - elapsed;
      }
    }
  }

  return maysleep;
}
