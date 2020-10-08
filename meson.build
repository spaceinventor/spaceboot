project('spaceboot', 'c', subproject_dir: 'lib', default_options: ['buildtype=release', 'c_std=gnu99', 'b_lto=false', 'csp:packet_padding_bytes=42', 'csp:use_qos=false', 'param:list_dynamic=true'])
 
csp = subproject('csp')
csp_dep = csp.get_variable('csp_dep')

param = subproject('param')
param_dep = param.get_variable('param_dep_whole')

sources = [
	'src/spaceboot.c',
]

spaceboot = executable('spaceboot', sources,
	dependencies : [csp_dep, param_dep],
	link_args : ['-Wl,-Map=spaceboot.map', '-lm'],
	install : true,
)

install_man('spaceboot.1')

custom_target('size', output: ['dummy.txt'], command: [find_program('size'), spaceboot.full_path()], depends: spaceboot, build_by_default: true)
