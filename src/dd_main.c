/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
#include <arpa/inet.h>
#include <stdio.h>

#include "dd_coap.h"
#include "dd_main.h"
#include "dd_resources.h"
#include "dd_storage.h"
#include "dd_types.h"

static struct { coap_context_t *context; } state;

/*
 * Initialize dotdot internal state
 */
void dd_init() {
  // initialize persistent storage
  dd_storage_init();
  dd_storage_link(__device);

  // initialize libcoap
  coap_startup();

  // set log levels
  coap_dtls_set_log_level(LOG_ERR);
  coap_set_log_level(LOG_ERR);

  // create libcoap context
  state.context = coap_new_context(0);

  // create default resource
  struct coap_resource_t *resource =
      coap_resource_unknown_init(dd_handle_root); // registers put

  // register handler for all required request types
  coap_register_handler(resource, COAP_REQUEST_DELETE, dd_handle_root);
  coap_register_handler(resource, COAP_REQUEST_GET, dd_handle_root);
  coap_register_handler(resource, COAP_REQUEST_POST, dd_handle_root);
  // coap_register_handler(resource, COAP_REQUEST_PUT, dd_handle_root); //
  // already registered

  // register resource
  coap_add_resource(state.context, resource);
}

/*
 * start dotdot coap server at port 5683
 */
void dd_start() {
  // declare listen addresses
  struct coap_address_t coap_unicast_addr;
  coap_address_init(&coap_unicast_addr);
  coap_unicast_addr.addr.sin6.sin6_family = AF_INET6;
  coap_unicast_addr.addr.sin6.sin6_port = htons(5683);
  coap_unicast_addr.addr.sin6.sin6_addr = in6addr_any;
  struct coap_address_t coap_multicast_realm_addr;
  coap_address_init(&coap_multicast_realm_addr);
  coap_multicast_realm_addr.addr.sin6.sin6_family = AF_INET6;
  coap_multicast_realm_addr.addr.sin6.sin6_port = htons(5683);
  inet_pton(AF_INET6, "ff03::00fd",
            &coap_multicast_realm_addr.addr.sin6.sin6_addr);
  struct coap_address_t coap_multicast_admin_addr;
  coap_address_init(&coap_multicast_admin_addr);
  coap_multicast_admin_addr.addr.sin6.sin6_family = AF_INET6;
  coap_multicast_admin_addr.addr.sin6.sin6_port = htons(5683);
  inet_pton(AF_INET6, "ff04::00fd",
            &coap_multicast_admin_addr.addr.sin6.sin6_addr);
  struct coap_address_t coap_multicast_site_addr;
  coap_address_init(&coap_multicast_site_addr);
  coap_multicast_site_addr.addr.sin6.sin6_family = AF_INET6;
  coap_multicast_site_addr.addr.sin6.sin6_port = htons(5683);
  inet_pton(AF_INET6, "ff05::00fd",
            &coap_multicast_site_addr.addr.sin6.sin6_addr);
  // TODO: review 00fd address ("All CoAP Nodes"):
  // https://www.iana.org/assignments/ipv6-multicast-addresses/ipv6-multicast-addresses.xhtml

  // create coap endpoints for all listen addresses
  struct coap_endpoint_t *coap_ep =
      coap_new_endpoint(state.context, &coap_unicast_addr, COAP_PROTO_UDP);
  struct coap_endpoint_t *coap_mc_rl_ep = coap_new_endpoint(
      state.context, &coap_multicast_realm_addr, COAP_PROTO_UDP);
  struct coap_endpoint_t *coap_mc_al_ep = coap_new_endpoint(
      state.context, &coap_multicast_admin_addr, COAP_PROTO_UDP);
  struct coap_endpoint_t *coap_mc_sl_ep = coap_new_endpoint(
      state.context, &coap_multicast_site_addr, COAP_PROTO_UDP);
}

/*
 * start secure dotdot coap server at port 5684
 */
void dd_start_secure() {
  // declare listen addresses
  struct coap_address_t coaps_unicast_addr;
  coap_address_init(&coaps_unicast_addr);
  coaps_unicast_addr.addr.sin6.sin6_family = AF_INET6;
  coaps_unicast_addr.addr.sin6.sin6_port = htons(5684);
  coaps_unicast_addr.addr.sin6.sin6_addr = in6addr_any;
  struct coap_address_t coaps_multicast_realm_addr;
  coap_address_init(&coaps_multicast_realm_addr);
  coaps_multicast_realm_addr.addr.sin6.sin6_family = AF_INET6;
  coaps_multicast_realm_addr.addr.sin6.sin6_port = htons(5684);
  inet_pton(AF_INET6, "ff03::00fd",
            &coaps_multicast_realm_addr.addr.sin6.sin6_addr);
  struct coap_address_t coaps_multicast_admin_addr;
  coap_address_init(&coaps_multicast_admin_addr);
  coaps_multicast_admin_addr.addr.sin6.sin6_family = AF_INET6;
  coaps_multicast_admin_addr.addr.sin6.sin6_port = htons(5684);
  inet_pton(AF_INET6, "ff04::00fd",
            &coaps_multicast_admin_addr.addr.sin6.sin6_addr);
  struct coap_address_t coaps_multicast_site_addr;
  coap_address_init(&coaps_multicast_site_addr);
  coaps_multicast_site_addr.addr.sin6.sin6_family = AF_INET6;
  coaps_multicast_site_addr.addr.sin6.sin6_port = htons(5684);
  inet_pton(AF_INET6, "ff05::00fd",
            &coaps_multicast_site_addr.addr.sin6.sin6_addr);
  // TODO: review 00fd address ("All CoAP Nodes"):
  // https://www.iana.org/assignments/ipv6-multicast-addresses/ipv6-multicast-addresses.xhtml

  // HACK set DTLS key
  uint8_t key = 'b'; // TODO: use proper key and server hint
  coap_context_set_psk(state.context, "", &key, 1);

  // create coap endpoints for all listen addresses
  struct coap_endpoint_t *coaps_ep =
      coap_new_endpoint(state.context, &coaps_unicast_addr, COAP_PROTO_DTLS);
  struct coap_endpoint_t *coaps_mc_rl_ep = coap_new_endpoint(
      state.context, &coaps_multicast_realm_addr, COAP_PROTO_DTLS);
  struct coap_endpoint_t *coaps_mc_al_ep = coap_new_endpoint(
      state.context, &coaps_multicast_admin_addr, COAP_PROTO_DTLS);
  struct coap_endpoint_t *coaps_mc_sl_ep = coap_new_endpoint(
      state.context, &coaps_multicast_site_addr, COAP_PROTO_DTLS);
}

/*
 * process (some) dotdot messages
 */
int dd_process_incoming(uint32_t timeout) {
  int ret = coap_io_process(state.context, timeout);
  if (ret == -1) {
    fprintf(stderr, "encountered error in libcoap while processing IO ...!\n");
    return -1;
  }

  return 0;
}

int32_t dd_process_outgoing() {
  return dd_process_bindings(state.context, __device);
}
