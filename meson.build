project('ds', 'c', default_options : [ 'c_std=c11' ])

cc = meson.get_compiler('c')
m = cc.find_library('m')

incdir = include_directories('include')

ds_vec = library('ds-vec', 'src/vec.c', include_directories : incdir)
ds_vec_dep = declare_dependency(link_with : ds_vec, include_directories : incdir)

ds_hashmap = library('ds-hashmap', 'src/hashmap.c', include_directories : incdir)
ds_hashmap_dep = declare_dependency(link_with : ds_hashmap, include_directories : incdir)

ds_tree = library('ds-tree', 'src/tree.c', include_directories : incdir)
ds_tree_dep = declare_dependency(link_with : ds_tree, include_directories : incdir)

ds_iter = library(
  'ds-iter', 'src/iter.c',
  dependencies: ds_vec_dep,
  include_directories : incdir)
ds_iter_dep = declare_dependency(link_with : ds_iter, include_directories : incdir)

ds_matrix = library(
  'ds-matrix',
  'src/matrix.c',
  dependencies : m,
  include_directories : incdir)
ds_matrix_dep = declare_dependency(link_with : ds_matrix, include_directories : incdir)

foreach item : [
  { 'c': 'src/test/container.c', 'd': [ ds_tree_dep ] },
  { 'c': 'src/test/iter.c', 'd': [ ds_iter_dep ] },
  { 'c': 'src/test/hashmap.c', 'd': [ ds_hashmap_dep ] },
  { 'c': 'src/bench/hashmap.c', 'd': [ ds_hashmap_dep ] },
]
  path = item.get('c')
  exe = executable(
    'exe-' + path.underscorify(),
    path,
    include_directories: incdir,
    dependencies: item.get('d'),
    c_args: [ '-DDS_DEBUG' ],
  )
  test(
    'test-' + path.underscorify(),
    exe
  )
endforeach
