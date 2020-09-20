/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
#ifndef HAVE_DDSETUP_H
#define HAVE_DDSETUP_H

#include <stdint.h>

/*
 * Initialize dotdot internal state
 */
void dd_init();

/*
 * start dotdot coap server at port 5683
 */
void dd_start();

/*
 * start secure dotdot coap server at port 5684
 */
void dd_start_secure();

/*
 * spend up to timeout milliseconds processing incoming messages
 *
 * returns -1 on error
 */
int dd_process_incoming(uint32_t timeout);

/*
 * process outgoing messages (bindings)
 *
 * returns time in seconds system may sleep; -1 on error
 */
int32_t dd_process_outgoing();

#endif /* HAVE_DDSETUP_H */
