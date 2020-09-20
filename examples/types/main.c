/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

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

static bool boolval = false;
bool endpoint_1_cluster_s2_attribute_0_handle_read() { return boolval; }
void endpoint_1_cluster_s2_attribute_0_handle_write(bool value) {
  boolval = value;
}

static int8_t int8val = 0;
int8_t endpoint_1_cluster_s2_attribute_1_handle_read() { return int8val; }
void endpoint_1_cluster_s2_attribute_1_handle_write(int8_t value) {
  int8val = value;
}

static int16_t int16val = 0;
int16_t endpoint_1_cluster_s2_attribute_2_handle_read() { return int16val; }
void endpoint_1_cluster_s2_attribute_2_handle_write(int16_t value) {
  int16val = value;
}

static int32_t int32val = 0;
int32_t endpoint_1_cluster_s2_attribute_3_handle_read() { return int32val; }
void endpoint_1_cluster_s2_attribute_3_handle_write(int32_t value) {
  int32val = value;
}

static uint8_t uint8val = 0;
uint8_t endpoint_1_cluster_s2_attribute_4_handle_read() { return uint8val; }
void endpoint_1_cluster_s2_attribute_4_handle_write(uint8_t value) {
  uint8val = value;
}

static uint16_t uint16val = 0;
uint16_t endpoint_1_cluster_s2_attribute_5_handle_read() { return uint16val; }
void endpoint_1_cluster_s2_attribute_5_handle_write(uint16_t value) {
  uint16val = value;
}

static uint32_t uint32val = 0;
uint32_t endpoint_1_cluster_s2_attribute_6_handle_read() { return uint32val; }
void endpoint_1_cluster_s2_attribute_6_handle_write(uint32_t value) {
  uint32val = value;
}

char stringval[1024] = {0};
const char *endpoint_1_cluster_s2_attribute_7_handle_read() {
  return stringval;
}
void endpoint_1_cluster_s2_attribute_7_handle_write(const char *value) {
  strncpy(stringval, value, sizeof(stringval) - 1);
}

time_t timeval = 0;
time_t endpoint_1_cluster_s2_attribute_8_handle_read() { return timeval; }
void endpoint_1_cluster_s2_attribute_8_handle_write(time_t value) {
  timeval = value;
}

void endpoint_1_cluster_s2_handle_notification(dd_notification *notification) {
  printf("Endpoint 1 Cluster 1 notification received\n");
}
