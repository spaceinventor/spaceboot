project('spaceboot', 'c', subproject_dir: 'lib', default_options: ['buildtype=release', 'c_std=gnu99'])

csp = subproject('csp')
csp_dep = csp.get_variable('csp_dep')

param = subproject('param')
param_dep = param.get_variable('param_dep')

sources = [
	'src/spaceboot.c',
]

spaceboot = executable('spaceboot', sources,
	dependencies : [csp_dep, param_dep],
	link_args : ['-Wl,-Map=spaceboot.map', '-lm'],
	c_args : ['-DIMAGEDIR="../images"'],
	install : true,
)

install_man('spaceboot.1')

custom_target('size', output: ['dummy.txt'], command: [find_program('size'), spaceboot.full_path()], depends: spaceboot, build_by_default: true)
