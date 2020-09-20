# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.
types_restree = custom_target(
    'resource_tree.c',
    output: ['resource_tree.c', 'resource_tree.h'],
    input: 'zcl.xml',
    command: [python, libdd_generator, '--schema', libdd_schema, '--input', '@INPUT@', '--source', '@OUTPUT0@', '--header', '@OUTPUT1@'],
)
executable('types', ['main.c', types_restree], link_with : libdd, include_directories: libdd_include)
