#!/bin/bash
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

# helper
function eexec() {
	echo $* 1>&2
	$*
	return $?
}
function decode() {
	# cut off line feed and convert to json
	head --bytes -1 | json_xs -f cbor -t json-pretty
	return $?
}

# create reporting configuration for cluster s3 attribute 0
# cbor.me: {"n": 10, "x": 30, "a": {0: {}}}
# cbor.me: {'n': 10, 'x': 30, 'a': {0: {}}}
printf "\xA3\x61\x6E\x0A\x61\x78\x18\x1E\x61\x61\xA1\x00\xA0" | eexec coap-client -m post -f - coap://[::1]/zcl/e/1/s3/r
eexec coap-client -m get coap://[::1]/zcl/e/1/s3/r/1 | decode

# create binding to self
# cbor.me: {"r": 1,"u": "coap://localhost/zcl/e/1/s3/n"}
printf "\xA2\x61\x72\x01\x61\x75\x78\x1Dcoap://localhost/zcl/e/1/s3/n" | eexec coap-client -m post -f - coap://[::1]/zcl/e/1/s3/b
eexec coap-client -m get coap://[::1]/zcl/e/1/s3/b/1 | decode

# create binding to fictitious server at localhost:1234
# cbor.me: {"r": 1,"u": "coap://localhost:1234/"}
printf "\xA2\x61\x72\x01\x61\x75\x76coap://localhost:1234/" | eexec coap-client -m post -f - coap://[::1]/zcl/e/1/s3/b
eexec coap-client -m get coap://[::1]/zcl/e/1/s3/b/2 | decode
