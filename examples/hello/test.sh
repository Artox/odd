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

eexec coap-client -m get coap://[::1]/zcl | decode
eexec coap-client -m get coap://[::1]/zcl/e | decode
eexec coap-client -m get coap://[::1]/zcl/e/1 | decode
eexec coap-client -m get coap://[::1]/zcl/e/1/s1 | decode
eexec coap-client -m get coap://[::1]/zcl/e/1/s1/a | decode
eexec coap-client -m get coap://[::1]/zcl/e/1/s1/a/0 | decode
