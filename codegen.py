#!/usr/bin/env python3
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

import argparse
import xmlschema

def header(output,header,input):
  print("/*",file=header)
  print(" * ZCL Resource Tree",file=header)
  print(" *",file=header)
  print(" * This file was automatically generated from %s, do not edit!"% (input),file=header)
  print(" */",file=header)
  print(file=header)
  print("#include <dd_types.h>",file=header)

  print("/*",file=output)
  print(" * ZCL Resource Tree",file=output)
  print(" *",file=output)
  print(" * This file was automatically generated from %s, do not edit!"% (input),file=output)
  print(" */",file=output)
  print(file=output)
  print("#include \"resource_tree.h\"",file=output)

def declare_device(output,header):
  print(file=output)
  print("// device (root)",file=output)
  print("static dd_device device = {",file=output)
  print("\t.endpoints = endpoints,",file=output)
  print("\t.endpoints_length = sizeof(endpoints) / sizeof(dd_endpoint *),",file=output)
  print("};",file=output)
  print("dd_device *__device = &device;",file=output)

def declare_endpoint(output,header,eid):
  print(file=output)
  print("// endpoint %x"% (eid),file=output)
  print("static dd_endpoint endpoint_%x = {"% (eid),file=output)
  print("\t.id = 0x%x,"% (eid),file=output)
  print("\t.cluster = endpoint_%x_cluster,"% (eid),file=output)
  print("\t.cluster_length = sizeof(endpoint_%x_cluster) / sizeof(dd_cluster *),"% (eid),file=output)
  print("};",file=output)
  return ("endpoint_%x"% (eid))

def declare_endpoint_list(output,header,endpoints):
  print(file=output)
  print("// endpoints",file=output)
  print("static dd_endpoint *endpoints[%i] = {"% (len(endpoints)),file=output)
  for endpoint in endpoints:
    print("\t&%s,"% endpoint,file=output)
  print("};",file=output)

def declare_cluster(output,header,eid,cl,role):
  print(file=header)
  print("// endpoint %x cluster %c%x"% (eid,role[0],cl),file=header)
  print("void endpoint_%x_cluster_%c%x_handle_notification(dd_notification *notification);"% (eid,role[0],cl),file=header)

  print(file=output)
  print("// endpoint %x cluster %c%x"% (eid,role[0],cl),file=output)
  print("static dd_cluster endpoint_%x_cluster_%c%x = {"% (eid,role[0],cl),file=output)
  print("\t.id = 0x%x,"% (cl),file=output)
  print("\t.role = '%c',"% (role[0]),file=output)
  print("\t.manufacturer = %i,"% (0),file=output)
  print("\t.attributes = endpoint_%x_cluster_%c%x_attributes,"% (eid,role[0],cl),file=output)
  print("\t.attributes_length = sizeof(endpoint_%x_cluster_%c%x_attributes) / sizeof(dd_attribute *),"% (eid,role[0],cl),file=output)
  print("\t.bindings = {0},",file=output)
  print("\t.bindings_length = 0,",file=output)
  print("\t.commands = endpoint_%x_cluster_%c%x_commands,"% (eid,role[0],cl),file=output)
  print("\t.commands_length = sizeof(endpoint_%x_cluster_%c%x_commands) / sizeof(dd_command *),"% (eid,role[0],cl),file=output)
  print("\t.reports = {0},",file=output)
  print("\t.reports_length = 0,",file=output)
  print("\t.notify = endpoint_%x_cluster_%c%x_handle_notification,"% (eid,role[0],cl),file=output)
  print("};",file=output)
  return ("endpoint_%x_cluster_%c%x"% (eid,role[0],cl))

def declare_cluster_list(output,header,eid,cluster):
  print(file=output)
  print("// endpoint 0x%x cluster"% (eid),file=output)
  print("static dd_cluster *endpoint_%x_cluster[%i] = {"% (eid,len(cluster)),file=output)
  for cluster_ in cluster:
    print("\t&%s,"% cluster_,file=output)
  print("};",file=output)

def declare_attribute_handler(output,header,eid,cl,role,aid,name,type):
  signatures={ # this is a shortcut, ZCL schema does declare each type in detail ...
    "bool": "bool ",
    "int8": "int8_t ",
    "int16": "int16_t ",
    "int32": "int32_t ",
    "uint8": "uint8_t ",
    "uint16": "uint16_t ",
    "uint32": "uint32_t ",
    "string": "const char *",
    "UTC": "time_t ",
  }
  print(file=header)
  print("// endpoint %x cluster %c%x attribute %x read+write handler"% (eid,role[0],cl,aid),file=header)
  print("%sendpoint_%x_cluster_%c%x_attribute_%x_handle_read();"% (signatures[type],eid,role[0],cl,aid),file=header)
  print("void endpoint_%x_cluster_%c%x_attribute_%x_handle_write(%svalue);"% (eid,role[0],cl,aid,signatures[type]),file=header)

  print(file=output)
  print("// endpoint %x cluster %c%x attribute %x read handler"% (eid,role[0],cl,aid),file=output)
  print("static dd_value *endpoint_%x_cluster_%c%x_attribute_%x_handle_read_wrapper(void *buffer, size_t buffer_size) {"% (eid,role[0],cl,aid),file=output)
  print("\t%sres = endpoint_%x_cluster_%c%x_attribute_%x_handle_read();"% (signatures[type],eid,role[0],cl,aid),file=output)
  print("\treturn dd_%s_to_value(res, buffer, buffer_size);"% (type),file=output)
  print("};",file=output)
  print(file=output)
  print("// endpoint %x cluster %c%x attribute %x write handler"% (eid,role[0],cl,aid),file=output)
  print("static void endpoint_%x_cluster_%c%x_attribute_%x_handle_write_wrapper(dd_value *value) {"% (eid,role[0],cl,aid),file=output)
  print("\t%sarg = dd_value_to_%s(value);"% (signatures[type],type),file=output)
  print("\tendpoint_%x_cluster_%c%x_attribute_%x_handle_write(arg);"% (eid,role[0],cl,aid),file=output)
  print("};",file=output)

def declare_attribute(output,header,eid,cl,role,aid,name):
  print(file=output)
  print("// endpoint %x cluster %c%x attribute %x"% (eid,role[0],cl,aid),file=output)
  print("static dd_attribute endpoint_%x_cluster_%c%x_attribute_%x = {"% (eid,role[0],cl,aid),file=output)
  print("\t.id = 0x%x,"% (aid),file=output)
  print("\t.name = \"%s\","% (name),file=output)
  print("\t.read = endpoint_%x_cluster_%c%x_attribute_%x_handle_read_wrapper,"% (eid,role[0],cl,aid),file=output)
  print("\t.write = endpoint_%x_cluster_%c%x_attribute_%x_handle_write_wrapper,"% (eid,role[0],cl,aid),file=output)
  print("};",file=output)
  return ("endpoint_%x_cluster_%c%x_attribute_%x"% (eid,role[0],cl,aid))

def declare_attribute_list(output,header,eid,cl,role,attributes):
  print(file=output)
  print("// endpoint 0x%x cluster %c%x attributes"% (eid,role[0],cl),file=output)
  print("static dd_attribute *endpoint_%x_cluster_%c%x_attributes[%i] = {"% (eid,role[0],cl,len(attributes)),file=output)
  for attribute in attributes:
    print("\t&%s,"% attribute,file=output)
  print("};",file=output)

def declare_command(output,header,eid,cl,role,cid):
  print(file=header)
  print("// endpoint %x cluster %c%x command %x"% (eid,role[0],cl,cid),file=header)
  print("void endpoint_%x_cluster_%c%x_command_%x_handle_exec();"% (eid,role[0],cl,cid),file=header)

  print(file=output)
  print("// endpoint %x cluster %c%x command %x"% (eid,role[0],cl,cid),file=output)
  print("static dd_command endpoint_%x_cluster_%c%x_command_%x = {"% (eid,role[0],cl,cid),file=output)
  print("\t.id = 0x%x,"% (cid),file=output)
  print("\t.exec = endpoint_%x_cluster_%c%x_command_%x_handle_exec,"% (eid,role[0],cl,cid),file=output)
  print("};",file=output)
  return ("endpoint_%x_cluster_%c%x_command_%x"% (eid,role[0],cl,cid))

def declare_command_list(output,header,eid,cl,role,commands):
  print(file=output)
  print("// endpoint 0x%x cluster %c%x commands"% (eid,role[0],cl),file=output)
  print("static dd_command *endpoint_%x_cluster_%c%x_commands[%i] = {"% (eid,role[0],cl,len(commands)),file=output)
  for command in commands:
    print("\t&%s,"% command,file=output)
  print("};",file=output)

# handle cli arguments
parser = argparse.ArgumentParser(description='Generate C structures from ZCL resource tree.')
parser.add_argument("--schema", required=True)
parser.add_argument("--input", required=True)
parser.add_argument("--source", required=True)
parser.add_argument("--header", required=True)
args = parser.parse_args()

# load schema
zcl = my_schema = xmlschema.XMLSchema(args.schema)

# load application description
app = zcl.to_dict(args.input)

# open output file
outsource = open(args.source, 'w')
outheader = open(args.header, 'w')

header(outsource,outheader,input)

endpoint_declarations = []
for endpoint in app["endpoint"]:
  eid = endpoint["@id"]

  cluster_declarations = []
  for cluster in endpoint["zcl:cluster"]:
    cl = int(cluster["@id"], base=16)

    for role in ["client", "server"]:
      if role in cluster:
        attribute_declarations = []
        if "attributes" in cluster[role]:
          for attribute in cluster[role]["attributes"]["attribute"]:
            aid = int(attribute["@id"], base=16)
            name = attribute["@name"]
            type = attribute["@type"]

            declare_attribute_handler(outsource,outheader,eid,cl,role,aid,name,type)

            attribute_declarations.append(declare_attribute(outsource,outheader,eid,cl,role,aid,name))

        declare_attribute_list(outsource,outheader,eid,cl,role,attribute_declarations)

        command_declarations = []
        if "commands" in cluster[role]:
          for command in cluster[role]["commands"]["command"]:
            cid = int(command["@id"], base=16)

            command_declarations.append(declare_command(outsource,outheader,eid,cl,role,cid))

        declare_command_list(outsource,outheader,eid,cl,role,command_declarations)

        cluster_declarations.append(declare_cluster(outsource,outheader,eid,cl,role))

  declare_cluster_list(outsource,outheader,eid,cluster_declarations)

  endpoint_declarations.append(declare_endpoint(outsource,outheader,eid))

declare_endpoint_list(outsource,outheader,endpoint_declarations)

declare_device(outsource,outheader)

outsource.close()
outheader.close()

print("Finished generating {\"%s\", \"%s\"} from %s with %s"% (args.source,args.header,args.input,args.schema))
