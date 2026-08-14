project('pharo-vm', 'c',
  meson_version: '>=1.1',
  version: '12.0.0',
  default_options: [
    'c_std=gnu11',
    'warning_level=0',
    'buildtype=release',
    'optimization=2',
  ],
)

flavour = get_option('flavour')

is_standalone = not meson.is_subproject()
is_core_static = get_option('default_library') == 'static'

cc = meson.get_compiler('c')
fs = import('fs')
pkg = import('pkgconfig')
python = import('python').find_installation()

word_size = cc.sizeof('void*') * 8
generated = get_option('generated_dir')
if generated == ''
  slang_dir = 'slang' / flavour
  run_command(python, files('tools/generate-sources.py'),
    '--checkout', meson.current_source_dir(),
    '--flavour', flavour,
    '--output', meson.current_source_dir() / slang_dir,
    '--cache', meson.current_source_dir() / 'slang/vmmaker',
    check: true)
  generated = slang_dir / 'generated' / word_size.to_string()
endif
generated_abs = meson.current_source_dir() / generated

host_os_family = host_machine.system()
host_os = host_machine.subsystem().split('-')[0]
is_darwin = host_os_family == 'darwin'
is_windows = host_os_family == 'windows'
is_apple_embedded = is_darwin and host_os != 'macos'
platform_dir = 'unix'
if is_darwin
  platform_dir = 'osx'
elif is_windows
  platform_dir = 'win'
endif

lsb_first = host_machine.endian() == 'little' ? 1 : 0
vm_version = '12.0.0'

conf = configuration_data()
conf.set('VM_NAME', 'Pharo')
conf.set('DEFAULT_IMAGE_NAME', 'Pharo.image')
conf.set('OS_TYPE', is_windows ? 'Win32' : 'unix')
conf.set('VM_TARGET', host_os_family)
conf.set('VM_TARGET_CPU', host_machine.cpu_family())
conf.set('VM_TARGET_OS', host_os_family)
vm_commit = run_command('git', '-C', meson.current_source_dir(),
  'log', '-1', '--format=%H', check: true).stdout().strip()
vm_commit_date = run_command('git', '-C', meson.current_source_dir(),
  'log', '-1', '--format=%cI', check: true).stdout().strip()
conf.set('BUILT_FROM', 'frida/pharo-vm @0@ - Commit: @1@ - Date: @2@'.format(
  vm_version, vm_commit, vm_commit_date))
conf.set('LSB_FIRST', lsb_first)
conf.set('ALWAYS_INTERACTIVE', 0)
conf.set('PharoVM_VERSION_MAJOR', 12)
conf.set('PharoVM_VERSION_MINOR', 0)
conf.set('PharoVM_VERSION_PATCH', 0)
conf.set('PharoVM_VERSION_STRING', vm_version)
conf.set('PHARO_VM_IN_WORKER_THREAD', 1)

foreach header : ['dirent.h', 'features.h', 'unistd.h', 'ndir.h', 'sys/ndir.h',
                  'sys/dir.h', 'sys/filio.h', 'sys/time.h', 'execinfo.h',
                  'dlfcn.h', 'sys/uuid.h', 'uuid/uuid.h', 'uuid.h']
  name = 'HAVE_' + header.to_upper().underscorify()
  conf.set(name, cc.has_header(header) ? 1 : false)
endforeach

foreach function : ['bzero', 'getcontext']
  conf.set('HAVE_' + function.to_upper(), cc.has_function(function) ? 1 : false)
endforeach

conf.set('HAVE_TM_GMTOFF',
  cc.has_member('struct tm', 'tm_gmtoff', prefix: '#include <time.h>') ? 1 : false)

foreach size : [['int', 'SIZEOF_INT'], ['long', 'SIZEOF_LONG'],
                ['long long', 'SIZEOF_LONG_LONG'], ['void*', 'SIZEOF_VOID_P']]
  conf.set(size[1], cc.sizeof(size[0]))
endforeach

apple_corefoundation = []
apple_security = []
openssl_deps = []
platform_deps = []
if is_darwin
  apple_corefoundation = [dependency('appleframeworks', modules: ['CoreFoundation'])]
  apple_security = [dependency('appleframeworks', modules: ['CoreFoundation', 'Security'])]
  frameworks = ['Foundation', 'CoreFoundation', 'CoreGraphics']
  if not is_apple_embedded
    frameworks += 'AppKit'
  endif
  platform_deps += dependency('appleframeworks', modules: frameworks)
endif

if is_windows
  foreach name : ['winmm', 'ws2_32', 'dbghelp', 'ole32', 'comctl32']
    platform_deps += cc.find_library(name)
  endforeach
endif

if not is_darwin and not is_windows
  openssl_deps = [dependency('openssl')]
endif

dl_dep = cc.find_library('dl', required: false)
m_dep = cc.find_library('m', required: false)
uuid_dep = cc.find_library('uuid', required: not (is_darwin or is_windows))
thread_dep = dependency('threads')
ffi_dep = dependency('libffi')
if is_windows
  ffi_library = cc.find_library('ffi', required: false)
  if not ffi_library.found()
    ffi_library = cc.find_library('libffi')
  endif
  ffi_dep = [ffi_dep, ffi_library]
endif

conf.set('HAVE_LIBDL', dl_dep.found() ? 1 : false)
conf.set('HAVE_UUID_GENERATE',
  cc.has_function('uuid_generate', dependencies: uuid_dep) ? 1 : false)
conf.set('HAVE_UUIDGEN',
  cc.has_function('uuidgen', dependencies: uuid_dep) ? 1 : false)

config_h = configure_file(
  input: 'meson-config.h.in',
  output: 'pharovmConfig.h',
  configuration: conf,
  format: 'cmake@',
  install: is_standalone,
  install_dir: get_option('includedir') / 'pharovm',
)


core_includes = [include_directories(
  '.',
  'include',
  'include/pharovm',
  'include/pharovm/common',
  'include/pharovm/ffi',
  'include/pharovm/semaphores',
  'include/pharovm/' + platform_dir,
  generated / 'vm/include',
)]

rewrite_generated_source = files('tools/rewrite-generated-source.py')

rewrite_rules = ['--rule=tolerate-relocated-regions',
                 '--rule=confine-pthread-to-apple',
                 '--rule=rename-config-header']

jit_backends = []
foreach backend : ['cogitX64SysV.c', 'cogitX64WIN64.c', 'cogitaarch64.c',
                   'cogitriscv64.c']
  if fs.is_file(generated / 'vm/src' / backend)
    jit_backends += custom_target(backend,
      input: generated / 'vm/src' / backend,
      output: backend,
      command: [python, rewrite_generated_source, '@INPUT@', '@OUTPUT@']
               + rewrite_rules)
  endif
endforeach

interpreter_names = flavour == 'CoInterpreter' \
  ? ['gcc3x-cointerp.c', 'cogit.c'] \
  : ['gcc3x-interp.c']

interpreter_sources = []
foreach name : interpreter_names
  interpreter_sources += custom_target(name,
    input: generated / 'vm/src' / name,
    output: name,
    command: [python, rewrite_generated_source, '@INPUT@', '@OUTPUT@']
             + rewrite_rules,
    depends: jit_backends)
endforeach

support_sources = files(
  'src/debug.c',
  'src/utils.c',
  'src/errorCode.c',
  'src/externalPrimitives.c',
  'src/client.c',
  'src/pathUtilities.c',
  'src/fileDialogCommon.c',
  'src/stringUtilities.c',
  'src/imageAccess.c',
  'src/memoryImage.c',
  'src/parameters/parameterVector.c',
  'src/parameters/parameters.c',
  'src/semaphores/platformSemaphore.c',
  'src/common/heartbeat.c',
  'src/common/sqHeapMap.c',
  'src/common/sqVirtualMachine.c',
  'src/common/sqNamedPrims.c',
  'src/common/staticPlugins.c',
  'src/common/sqExternalSemaphores.c',
  'src/common/sqTicker.c',
)

ffi_sources = files(
  'src/ffi/functionDefinitionPrimitives.c',
  'src/ffi/primitiveCalls.c',
  'src/ffi/primitiveUtils.c',
  'src/ffi/types.c',
  'src/ffi/typesPrimitives.c',
  'src/ffi/utils.c',
  'src/ffi/sameThread/sameThread.c',
  'src/ffi/callbacks/callbackPrimitives.c',
  'src/ffi/callbacks/callbacks.c',
  'src/semaphores/pharoSemaphore.c',
  'src/threadSafeQueue/threadSafeQueue.c',
  'src/ffi/pThreadedFFI.c',
  'src/ffi/worker/worker.c',
  'src/ffi/worker/workerPrimitives.c',
  'src/ffi/worker/workerTask.c',
)

platform_sources = files('src/unix/aio.c', 'src/unix/debugUnix.c',
                         'src/unix/memoryUnix.c', 'src/unix/fileDialogUnix.c')
if is_windows
  platform_sources = files('src/win/sqWin32SpurAlloc.c', 'src/win/aioWin.c',
                           'src/win/winDebug.c', 'src/win/winDebugMenu.c',
                           'src/win/winDebugWindow.c',
                           'src/win/fileDialogWin32.c')
elif is_darwin
  add_languages('objc', 'objcpp', native: false)
  platform_sources = files('src/osx/aioOSX.c', 'src/osx/utilsMac.mm',
                           'src/unix/debugUnix.c', 'src/parameters/parameters.m',
                           'src/unix/memoryUnix.c')
  platform_sources += is_apple_embedded \
    ? files('src/unix/fileDialogUnix.c') \
    : files('src/osx/fileDialogMac.m')
endif

common_args = [
  '-DIMMUTABILITY=1',
  '-DCOGMTVM=0',
  '-DDEBUGVM=0',
  '-D_FILE_OFFSET_BITS=64',
  '-DSOURCE_PATH_SIZE=0',
  '-DLSB_FIRST=@0@'.format(lsb_first),
]
if flavour == 'CoInterpreter'
  common_args += '-DASYNC_FFI_QUEUE=1'
endif
if is_windows
  common_args += ['-DUNICODE', '-D_UNICODE',
                  '-DWIN32=1', '-DWIN64=1', '-DWIN32_LEAN_AND_MEAN',
                  '-DNO_ISNAN', '-DAllocationCheckFiller=0xADD4E55',
                  '-D_WIN32_WINNT=0x0600', '-DWINVER=0x0600',
                  '-DNTDDI_VERSION=0x06000000']
endif
common_args += cc.get_supported_arguments(
  '-Wno-int-conversion',
  '-Wno-pointer-sign',
  '-Wno-pointer-to-int-cast',
  '-Wno-incompatible-pointer-types',
  '-Wno-implicit-function-declaration',
  '-Wno-sometimes-uninitialized',
  '-Wno-unknown-pragmas',
  '-Wno-deprecated-declarations',
  '-Wno-compare-distinct-pointer-types',
  '-Wno-format',
  '-Wno-macro-redefined',
  '-Wno-parentheses',
  '-Wno-unused-value',
  '-Wno-non-literal-null-conversion',
)

core_args = common_args + [
  '-DFEATURE_FFI=1',
  '-DFEATURE_THREADED_FFI=1',
  '-DUSE_INLINE_MEMORY_ACCESSORS=1',
]

if flavour == 'CoInterpreter'
  core_args += '-DFEATURE_JIT_SIMD=1'
endif

if is_darwin
  core_args += '-DREAD_ONLY_CODE_ZONE=1'
endif

setjmp_wrapper = []
if is_windows and host_machine.cpu_family() == 'aarch64'
  preprocessed = custom_target('setjmp-wrapper-preprocessed',
    input: 'src/utils/setjmp-Windows-wrapper-ARM64.S',
    output: 'setjmp-wrapper.asm',
    command: [cc.cmd_array(), '/nologo', '/P', '/EP', '/I' + meson.current_source_dir(),
              '@INPUT@', '/Fi@OUTPUT@'])
  setjmp_wrapper = custom_target('setjmp-wrapper',
    input: preprocessed,
    output: 'setjmp-wrapper.obj',
    command: [find_program('armasm64'), '/nologo', '/Fo', '@OUTPUT@', '@INPUT@'])
endif

socket_deps = is_windows ? [cc.find_library('ws2_32')] : []
tls_deps = openssl_deps
if is_darwin
  tls_deps = apple_security
elif is_windows
  tls_deps = [cc.find_library('crypt32'), cc.find_library('secur32')]
endif

plugins = [
  ['FilePlugin', true, true, [], apple_corefoundation],
  ['NewFilePlugin', true, true, [], []],
  ['FileAttributesPlugin', false, true, ['FilePlugin'], []],
  ['SocketPlugin', false, false, [], socket_deps],
  ['UUIDPlugin', false, false, [], [uuid_dep]],
  ['LocalePlugin', false, true, [], apple_corefoundation],
  ['LargeIntegers', false, false, [], []],
  ['MiscPrimitivePlugin', false, false, [], []],
  ['BitBltPlugin', false, false, [], []],
  ['SurfacePlugin', true, false, [], []],
  ['FloatArrayPlugin', true, false, [], []],
]

if not is_apple_embedded
  plugins += [
    ['JPEGReaderPlugin', false, false, [], []],
    ['JPEGReadWriter2Plugin', false, false, [], []],
    ['DSAPrims', false, false, [], []],
    ['B2DPlugin', false, false, [], []],
    ['SqueakSSL', false, false, [], tls_deps],
  ]
  if not is_windows
    plugins += [['UnixOSProcessPlugin', false, false,
                 ['FilePlugin', 'SocketPlugin'], []]]
  endif
endif

# SQUEAK_BUILTIN_PLUGIN gives Slang's primitives internal linkage, which
# SurfacePlugin's handwritten manual surfaces call from another TU. Fixable in
# Slang, not here.
unlinkable_plugins = ['SurfacePlugin']

static_plugins = get_option('static_plugins')
if is_core_static
  static_plugins = ['all']
  warning('Leaving out the plugins that cannot be linked into a static core: '
          + ', '.join(unlinkable_plugins))
endif
if static_plugins == ['all']
  static_plugins = []
  foreach plugin : plugins
    if not (plugin[0] in unlinkable_plugins)
      static_plugins += plugin[0]
    endif
  endforeach
endif

plugin_specs = []
plugin_include_dirs = {}

foreach plugin : plugins
  name = plugin[0]
  sources = []

  common = fs.is_dir('plugins' / name / 'src/common') \
    ? 'plugins' / name / 'src/common' \
    : 'plugins' / name / 'common'

  platform_dirs = [platform_dir]
  if is_darwin and plugin[2]
    platform_dirs += 'unix'
  endif

  source_dirs = [common]
  foreach where : platform_dirs
    source_dirs += 'plugins' / name / 'src' / where
  endforeach

  foreach dir : source_dirs
    if fs.is_dir(dir)
      sources += files(run_command(
        'sh', '-c', 'ls @0@/*.c 2>/dev/null || true'.format(
          meson.current_source_dir() / dir),
        check: true).stdout().strip().split('\n'))
    endif
  endforeach

  if plugin[1]
    sources += custom_target(name + '.c',
      input: generated / 'plugins/src' / name / (name + '.c'),
      output: name + '.c',
      command: [python, rewrite_generated_source, '@INPUT@', '@OUTPUT@']
               + rewrite_rules)
  endif

  include_dirs = ['plugins' / name / 'include/common']
  foreach where : platform_dirs
    include_dirs += 'plugins' / name / 'include' / where
  endforeach

  plugin_includes = [core_includes]
  foreach dir : include_dirs
    if fs.is_dir(dir)
      plugin_includes += include_directories(dir)
    endif
  endforeach

  foreach needed : plugin[3]
    plugin_includes += plugin_include_dirs[needed]
  endforeach
  plugin_include_dirs += {name: plugin_includes}

  plugin_specs += [[name, sources, plugin_includes, plugin[3], plugin[4]]]
endforeach

static_plugin_libraries = []
static_plugin_deps = []

foreach spec : plugin_specs
  if spec[0] in static_plugins
    registration = configure_file(
      input: 'tools/static-plugin-registration.c.in',
      output: spec[0] + '-static-plugin.c',
      configuration: {'PLUGIN_NAME': spec[0]},
    )
    static_plugin_libraries += static_library(spec[0], spec[1], registration,
      config_h,
      include_directories: spec[2],
      c_args: common_args + '-DSQUEAK_BUILTIN_PLUGIN=1',
      dependencies: [m_dep] + spec[4],
    )
    static_plugin_deps += spec[4]
  endif
endforeach

core = library('PharoVMCore',
  support_sources, ffi_sources, platform_sources, interpreter_sources, config_h,
  setjmp_wrapper,
  include_directories: core_includes,
  c_args: core_args,
  dependencies: [ffi_dep, thread_dep, dl_dep, m_dep] + platform_deps
                + static_plugin_deps,
  link_whole: static_plugin_libraries,
  install: is_standalone,
  install_rpath: is_darwin ? '@loader_path' : '',
)

core_dep_arguments = {
  'include_directories': core_includes,
  'sources': config_h,
  'dependencies': [ffi_dep, thread_dep, dl_dep, m_dep] + platform_deps
                  + static_plugin_deps,
}
core_dep = is_core_static \
  ? declare_dependency(link_whole: core, kwargs: core_dep_arguments) \
  : declare_dependency(link_with: core, kwargs: core_dep_arguments)
meson.override_dependency('pharo-vm', core_dep)

if not is_apple_embedded and not meson.is_subproject()
  executable('Pharo',
    is_windows ? files('src/win/win32Main.c') : files('src/unix/unixMain.c'),
    include_directories: core_includes,
    c_args: common_args,
    link_with: core,
    install: true,
    win_subsystem: 'windows',
    install_rpath: is_darwin ? '@executable_path/../lib' : '$ORIGIN/../lib',
  )
endif

built_plugins = {}

foreach spec : plugin_specs
  if not (spec[0] in static_plugins) and not is_core_static
    plugin_deps = [core_dep, m_dep] + spec[4]
    foreach needed : spec[3]
      if needed in built_plugins
        plugin_deps += built_plugins[needed]
      endif
    endforeach

    library = shared_library(spec[0], spec[1],
      include_directories: spec[2],
      c_args: common_args,
      dependencies: plugin_deps,
      install: true,
      install_rpath: is_darwin ? '@loader_path' : '',
    )
    built_plugins += {spec[0]: declare_dependency(
      link_with: library, include_directories: spec[2])}
  endif
endforeach

if is_standalone
  install_subdir('include/pharovm',
    install_dir: get_option('includedir'),
    exclude_files: ['config.h.in'],
  )
  install_data(
    run_command('sh', '-c', 'ls @0@/vm/include/*.h'.format(generated_abs),
      check: true).stdout().strip().split('\n'),
    install_dir: get_option('includedir') / 'pharovm',
  )

  pkg_libraries = [core]
  if is_core_static
    if is_windows and cc.get_argument_syntax() == 'msvc'
      pkg_libraries = ['/WHOLEARCHIVE:PharoVMCore.lib'] + pkg_libraries
    elif is_darwin
      pkg_libraries = ['-Wl,-force_load,${libdir}/libPharoVMCore.a'] \
        + pkg_libraries
    else
      pkg_libraries = ['-Wl,--whole-archive', '-L${libdir}', '-lPharoVMCore',
                       '-Wl,--no-whole-archive'] + pkg_libraries
    endif
  endif

  pkg.generate(
    libraries: pkg_libraries,
    name: 'pharo-vm',
    description: 'Pharo virtual machine core',
    subdirs: ['.', 'pharovm', 'pharovm/common', 'pharovm/' + platform_dir],
  )
endif
