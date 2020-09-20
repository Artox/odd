/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
#ifndef HAVE_DDCOAP_H
#define HAVE_DDCOAP_H

#include <coap2/coap.h>
#include <stdint.h>

/*
 * CoAP Abstraction
 */

typedef struct coap_address_t coap_address_t;

int dd_coap_resolve(coap_address_t *address, const char *host, uint16_t port);

#endif /* HAVE_DDCOAP_H */
