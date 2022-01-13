project('spaceboot', 'c', subproject_dir: 'lib', default_options: [
	'buildtype=release', 
	'c_std=gnu11', 
	'b_lto=false', 
	'default_library=static',
	'csp:packet_padding_bytes=42', 
	'csp:buffer_count=100',
	'csp:buffer_size=2048',
	'csp:conn_max=20',
	'csp:conn_rxqueue_len=20',
	'csp:qfifo_len=20',
	'csp:port_max_bind=16',
	'csp:use_promisc=false',
	'param:have_fopen=true', 
	'param:slash=false', 
	'param:commands=false',
	'param:scheduler=false',
	'param:commands_client=false',
	'param:scheduler_client=false',
	'param:collector=false', 
	'param:list_dynamic=true'])

csp = subproject('csp')
csp_dep = csp.get_variable('csp_dep')

param = subproject('param')
param_dep = param.get_variable('param_dep').as_link_whole()

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
