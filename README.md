# opendotdot

A free dotdot Framework. Documentation is currently available on the [RheinMain University of Applied Sciences Distributed Systems Lab Wiki](https://wwwvs.cs.hs-rm.de/vs-wiki/index.php/MP-SS20-02).

## Requirements:

- [meson](https://mesonbuild.com/)
- [libcoap](https://github.com/obgm/libcoap)
- libcoap */usr/bin/coap-{client,server}* (for examples):

      install -v -m755 -o root -g root examples/.libs/coap-client /usr/bin
      install -v -m755 -o root -g root examples/.libs/coap-server /usr/bin

- [QCBOR](https://github.com/laurencelundblade/QCBOR)
- */usr/lib/pkgconfig/qcbor.pc*:

      Name: qcbor
      Description: qcbor
      Version: 0.0.0
      Cflags: 
      Libs: -L/usr/lib -lqcbor

- [ZCL XML Schema](https://zigbeealliance.org/wp-content/uploads/2019/11/19-01018-000-zcl7.0-Dotdot.zip):

      unzip 19-01018-000-zcl7.0-Dotdot.zip "*.xml" "*.xsd" -x "__MACOSX/*"
      mv 19-01018-000-zcl7.0_release zcl

## Compile

    meson build; cd build
    ninja

## Examples

Note: Examples below save state to *data.bin* in the working directory; Deletion is sufficient for a clean start.

### Hello World

- start application

      ./build/examples/hello/hello

- run prepared queries

      ./examples/hello/test.sh

### Persistent Attributes and Data Types

- start application

      ./build/examples/types/types

- run prepared queries

      ./examples/types/test.sh

### Bindings

- start application

      ./build/examples/binding/binding &

- configure (and query) report configuration and bindings

      ./examples/binding/test.sh

- start coap-server to examine notifications

      coap-server -p 1234 -v 9
