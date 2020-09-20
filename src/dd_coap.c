/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
#ifdef __linux__
#include <netdb.h>
#include <stdio.h>
#endif

#include "dd_coap.h"

#ifdef __linux__
int dd_coap_resolve(coap_address_t *address, const char *host, uint16_t port) {
  assert(address != 0);
  struct addrinfo *info;
  struct addrinfo hints = {.ai_socktype = SOCK_DGRAM,
                           .ai_family = AF_INET | AF_INET6};

  int ret = getaddrinfo(host, 0, &hints, &info);
  if (ret != 0) {
    // no matter what went wrong, nothing to be done about it ...
    fprintf(stderr, "getaddrinfo(%s) failed: %s\n", host, gai_strerror(ret));
    return -1;
  }

  // blindly pick the first result
  coap_address_init(address);
  memcpy(&address->addr, info->ai_addr, info->ai_addrlen);
  address->size = info->ai_addrlen;
  coap_address_set_port(address, port);

  freeaddrinfo(info);
  return 0;
}
#endif
