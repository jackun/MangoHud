# Copyright © 2019 Intel Corporation

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

project('overlay-layer',
  ['c', 'cpp'],
  version : 'v0.0.1',
  license : 'MIT',
  meson_version : '>= 0.46',
  default_options : ['buildtype=debugoptimized', 'b_ndebug=if-release', 'c_std=c99', 'cpp_std=c++14']
)

cc = meson.get_compiler('c')
cpp = meson.get_compiler('cpp')
prog_python = import('python').find_installation('python3')

pre_args = [
  '-D__STDC_CONSTANT_MACROS',
  '-D__STDC_FORMAT_MACROS',
  '-D__STDC_LIMIT_MACROS',
  '-DPACKAGE_VERSION="@0@"'.format(meson.project_version()),
]

# Define DEBUG for debug builds only (debugoptimized is not included on this one)
if get_option('buildtype') == 'debug'
  pre_args += '-DDEBUG'
endif

# TODO: this is very incomplete
if ['linux', 'cygwin', 'gnu'].contains(host_machine.system())
  pre_args += '-D_GNU_SOURCE'
  pre_args += '-DHAVE_PTHREAD'
endif

# Check for GCC style atomics
if cc.compiles('''#include <stdint.h>
                  int main() {
                    struct {
                      uint64_t *v;
                    } x;
                    return (int)__atomic_load_n(x.v, __ATOMIC_ACQUIRE) &
                           (int)__atomic_add_fetch(x.v, (uint64_t)1, __ATOMIC_ACQ_REL);

                  }''',
               name : 'GCC atomic builtins')
  pre_args += '-DUSE_GCC_ATOMIC_BUILTINS'
endif

# Not in C99, needs POSIX
if cc.compiles('''
                 #define _GNU_SOURCE
                 #include <time.h>
                  int main() {
                    struct timespec ts;
                    return timespec_get(&ts, TIME_UTC);

                  }''',
               name : 'Supports timespec_get')
  pre_args += '-DHAVE_TIMESPEC_GET'
endif

# Check for GCC style builtins
foreach b : ['bswap32', 'bswap64', 'clz', 'clzll', 'ctz', 'expect', 'ffs',
             'ffsll', 'popcount', 'popcountll', 'unreachable']
  if cc.has_function(b)
    pre_args += '-DHAVE___BUILTIN_@0@'.format(b.to_upper())
  endif
endforeach

null_dep = dependency('', required : false)

vulkan_wsi_args = []
vulkan_wsi_deps = []

with_platform_x11 = true
with_platform_wayland = false
with_xlib_lease = true

dep_x11 = dependency('x11')
dep_xext = dependency('xext')
dep_xcb = dependency('xcb')
dep_x11_xcb = dependency('x11-xcb')
dep_xcb_dri2 = dependency('xcb-dri2', version : '>= 1.8')
dep_libdrm = dependency(
  'libdrm', version : '>=' + '2.4.81',
  required : true
)

pre_args += '-DHAVE_DRI3'
dep_xcb_dri3 = dependency('xcb-dri3')
dep_xcb_present = dependency('xcb-present')
# until xcb-dri3 has been around long enough to make a hard-dependency:
if (dep_xcb_dri3.version().version_compare('>= 1.13') and
  dep_xcb_present.version().version_compare('>= 1.13'))
pre_args += '-DHAVE_DRI3_MODIFIERS'
endif
dep_xcb_sync = dependency('xcb-sync')
dep_xshmfence = dependency('xshmfence', version : '>= 1.1')

if with_platform_x11
  vulkan_wsi_args += ['-DVK_USE_PLATFORM_XCB_KHR', '-DVK_USE_PLATFORM_XLIB_KHR']
  vulkan_wsi_deps += [
    dep_xcb,
    dep_x11_xcb,
    dep_xcb_dri2,
    dep_xcb_dri3,
    dep_xcb_present,
    dep_xcb_sync,
    dep_xshmfence,
  ]
endif
if with_platform_wayland
  dep_wayland_client = dependency('wayland-client', version : '>=1.11')
  vulkan_wsi_args += ['-DVK_USE_PLATFORM_WAYLAND_KHR']
  vulkan_wsi_deps += dep_wayland_client
endif

vulkan_wsi_args += '-DVK_USE_PLATFORM_DISPLAY_KHR'
vulkan_wsi_deps += [dep_libdrm]

if with_xlib_lease
  dep_xcb_xrandr = dependency('xcb-randr')
  dep_xlib_xrandr = dependency('xrandr', version : '>= 1.3')
  vulkan_wsi_args += '-DVK_USE_PLATFORM_XLIB_XRANDR_EXT'
  vulkan_wsi_deps += [dep_xcb_xrandr, dep_xlib_xrandr]
endif

inc_common = [
    include_directories('include'),
    include_directories('mesa'),
]

# Check for generic C arguments
c_args = []
foreach a : ['-Werror=implicit-function-declaration',
             '-Werror=missing-prototypes', '-Werror=return-type',
             '-Werror=incompatible-pointer-types',
             '-fno-math-errno',
             '-fno-trapping-math', '-Qunused-arguments']
  if cc.has_argument(a)
    c_args += a
  endif
endforeach

foreach a : ['missing-field-initializers', 'format-truncation']
  if cc.has_argument('-W' + a)
    c_args += '-Wno-' + a
  endif
endforeach

c_vis_args = []
if cc.has_argument('-fvisibility=hidden')
  c_vis_args += '-fvisibility=hidden'
endif

# Check for generic C++ arguments
cpp_args = []
foreach a : ['-Werror=return-type',
             '-fno-math-errno', '-fno-trapping-math',
             '-Qunused-arguments']
  if cpp.has_argument(a)
    cpp_args += a
  endif
endforeach

# For some reason, the test for -Wno-foo always succeeds with gcc, even if the
# option is not supported. Hence, check for -Wfoo instead.

foreach a : ['non-virtual-dtor', 'missing-field-initializers', 'format-truncation']
  if cpp.has_argument('-W' + a)
    cpp_args += '-Wno-' + a
  endif
endforeach

no_override_init_args = []
foreach a : ['override-init', 'initializer-overrides']
  if cc.has_argument('-W' + a)
    no_override_init_args += '-Wno-' + a
  endif
endforeach

cpp_vis_args = []
if cpp.has_argument('-fvisibility=hidden')
  cpp_vis_args += '-fvisibility=hidden'
endif

foreach a : pre_args
  add_project_arguments(a, language : ['c', 'cpp'])
endforeach
foreach a : c_args
  add_project_arguments(a, language : ['c'])
endforeach
foreach a : cpp_args
  add_project_arguments(a, language : ['cpp'])
endforeach

# check for dl support
if cc.has_function('dlopen')
  dep_dl = null_dep
else
  dep_dl = cc.find_library('dl')
endif

dep_pthread = cc.find_library('pthread')

git_sha1_gen_py = files('bin/git_sha1_gen.py')
sha1_h = custom_target(
  'git_sha1.h',
  output : 'git_sha1.h',
  command : [prog_python, git_sha1_gen_py, '--output', '@OUTPUT@'],
  build_always : true, # commit sha1 can change without having touched these files
)

vk_layer_table_helpers = []
loader_genvk_py = files('3rdparty/Vulkan-Loader/scripts/loader_genvk.py')
foreach s : ['vk_dispatch_table_helper.h', 'vk_layer_dispatch_table.h']#, 'vk_loader_extensions.h', 'vk_loader_extensions.c']
  vk_layer_table_helpers += custom_target(
    s, output : s,
    command : [prog_python, loader_genvk_py,
      '-scripts', '../../Vulkan-Docs/scripts', # relative to loader_genvk.py
      '-registry', join_paths(meson.source_root(), '3rdparty/Vulkan-Docs/xml/vk.xml'),
      '-o','@OUTDIR@', s])
endforeach

vk_api_xml = files('3rdparty/Vulkan-Docs/xml/vk.xml')
vk_enum_to_str = custom_target(
  'vk_enum_to_str',
  input : ['bin/gen_enum_to_str.py', vk_api_xml],
  output : ['vk_enum_to_str.c', 'vk_enum_to_str.h'],
  command : [
    prog_python, '@INPUT0@', '--xml', '@INPUT1@',
    '--outdir', meson.current_build_dir()
  ],
)

util_files = files(
  'mesa/util/hash_table.c',
  'mesa/util/os_socket.c',
  'mesa/util/os_time.c',
  'mesa/util/ralloc.c',
  'mesa/main/hash.c',
)

subdir('3rdparty/imgui')
subdir('overlay-layer')