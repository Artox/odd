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

# Note: Use http://cbor.me/ to encode payloads ...

eexec coap-client -m get coap://[::1]/zcl | decode
eexec coap-client -m get coap://[::1]/zcl/e | decode
eexec coap-client -m get coap://[::1]/zcl/e/1 | decode
eexec coap-client -m get coap://[::1]/zcl/e/1/s2 | decode
eexec coap-client -m get coap://[::1]/zcl/e/1/s2/a | decode

# get + set + get attributes

# bool {1: true}
eexec coap-client -m get coap://[::1]/zcl/e/1/s2/a/0 | decode
printf "\xA1\x00\xF5" | eexec coap-client -m put -f - coap://[::1]/zcl/e/1/s2/a/0
eexec coap-client -m get coap://[::1]/zcl/e/1/s2/a/0 | decode

# int8 {1: -9}
eexec coap-client -m get coap://[::1]/zcl/e/1/s2/a/1 | decode
printf "\xA1\x01\x28" | eexec coap-client -m put -f - coap://[::1]/zcl/e/1/s2/a/1
eexec coap-client -m get coap://[::1]/zcl/e/1/s2/a/1 | decode

# int16 {2: -129}
eexec coap-client -m get coap://[::1]/zcl/e/1/s2/a/2 | decode
printf "\xA1\x02\x38\x80" | eexec coap-client -m put -f - coap://[::1]/zcl/e/1/s2/a/2
eexec coap-client -m get coap://[::1]/zcl/e/1/s2/a/2 | decode

# int32 {3: 100000}
eexec coap-client -m get coap://[::1]/zcl/e/1/s2/a/3 | decode
printf "\xA1\x02\x3A\x00\x01\x86\x9F" | eexec coap-client -m put -f - coap://[::1]/zcl/e/1/s2/a/3
eexec coap-client -m get coap://[::1]/zcl/e/1/s2/a/3 | decode

# uint8 {4: 250}
eexec coap-client -m get coap://[::1]/zcl/e/1/s2/a/4 | decode
printf "\xA1\x04\x18\xFA" | eexec coap-client -m put -f - coap://[::1]/zcl/e/1/s2/a/4
eexec coap-client -m get coap://[::1]/zcl/e/1/s2/a/4 | decode

# uint16 {5: 55000}
eexec coap-client -m get coap://[::1]/zcl/e/1/s2/a/5 | decode
printf "\xA1\x05\x19\xD6\xD8" | eexec coap-client -m put -f - coap://[::1]/zcl/e/1/s2/a/5
eexec coap-client -m get coap://[::1]/zcl/e/1/s2/a/5 | decode

# uint32 {6: 3000000000}
eexec coap-client -m get coap://[::1]/zcl/e/1/s2/a/6 | decode
printf "\xA1\x06\x1A\xB2\xD0\x5E\x00" | eexec coap-client -m put -f - coap://[::1]/zcl/e/1/s2/a/6
eexec coap-client -m get coap://[::1]/zcl/e/1/s2/a/6 | decode

# string {7: "G'Day"}
eexec coap-client -m get coap://[::1]/zcl/e/1/s2/a/7 | decode
printf "\xA1\x07\x65G'Day" | eexec coap-client -m put -f - coap://[::1]/zcl/e/1/s2/a/7
eexec coap-client -m get coap://[::1]/zcl/e/1/s2/a/7 | decode

# UTC {8: 1(1600105847)}
eexec coap-client -m get coap://[::1]/zcl/e/1/s2/a/8 | decode
printf "\xA1\x08\xC1\x1A\x5F\x5F\xAD\x77" | eexec coap-client -m put -f - coap://[::1]/zcl/e/1/s2/a/8
eexec coap-client -m get coap://[::1]/zcl/e/1/s2/a/8 | decode
