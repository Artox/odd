# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.
libcoap = dependency('libcoap-2-openssl')
libqcbor = dependency('qcbor')

libdd_sources = [
	'dd_cbor.c', 'dd_cbor.h',
	'dd_coap.c', 'dd_coap.h',
	'dd_main.c', 'dd_main.h',
	'dd_resources.c', 'dd_resources.h',
	'dd_storage.c', 'dd_storage.h',
	'dd_types.c', 'dd_types.h',
]
libdd = static_library('dd', libdd_sources, dependencies: [libcoap, libqcbor])

# save location of headers
libdd_include = include_directories('.')
