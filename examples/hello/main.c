/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
#include <assert.h>
#include <signal.h>
#include <stdio.h>

#include <dd_main.h>
#include <dd_types.h>

// shutdown signal handler
static volatile int exit_ = 0;
void on_signal(int s) { exit_ = 1; }

int main(int argc, char *argv[]) {
  // start dotdot
  dd_init();
  dd_start();
  dd_start_secure();

  // process dotdot
  while (!exit_) {
    int32_t sleep = dd_process_outgoing();
    if (sleep == -1)
      break;
    assert(sleep >= 0);

    int ret = dd_process_incoming(sleep);
    if (ret == -1)
      break;
  }

  // exit
  return 0;
}

const char *endpoint_1_cluster_s1_attribute_0_handle_read() {
  return "Hello, World!";
}

void endpoint_1_cluster_s1_attribute_0_handle_write(const char *value) {
  // not implemented
}

void endpoint_1_cluster_s1_handle_notification(dd_notification *notification) {
  // not implemented
}
