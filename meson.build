project('odd', 'c')

python = import('python').find_installation('python3', modules: ['xmlschema'])
libdd_generator = files('codegen.py')
libdd_schema = files('zcl.xsd')

subdir('src')
subdir('examples')
