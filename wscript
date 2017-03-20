#!/usr/bin/env python
# encoding: utf-8

from waftools import eclipse

APPNAME = 'spaceboot'
VERSION = '1'

top = '.'
out = 'build'

modules = ['lib/csp', 'lib/slash', 'lib/param']

def options(ctx):
    ctx.load('eclipse')
    ctx.recurse(modules)

def configure(ctx):
    ctx.load('eclipse')

    # CSP options
    ctx.options.disable_stlib = True
    ctx.options.enable_if_can = True
    ctx.options.enable_can_socketcan = True
    ctx.options.enable_crc32 = True
    ctx.options.enable_rdp = True
    
    ctx.options.slash_csp = True
    
    ctx.options.rparam_client = True
    ctx.options.rparam_client_slash = True
    ctx.options.param_server = True

    ctx.options.vmem_client = True
    ctx.options.vmem_client_ftp = True

    ctx.recurse(modules)
    
    ctx.env.prepend_value('CFLAGS', ['-Os','-Wall', '-g', '-std=gnu99'])

def build(ctx):
    ctx.recurse(modules)
    ctx.program(
        target   = APPNAME,
        source   = ctx.path.ant_glob('src/*.c'),
        use      = ['csp', 'slash', 'param', 'vmem'],
        lib      = ['pthread', 'm'] + ctx.env.LIBS,
        ldflags  = '-Wl,-Map=' + APPNAME + '.map')

def dist(ctx):
    ctx.algo      = 'tar.xz'
    ctx.excl      = '**/.* build'

    
