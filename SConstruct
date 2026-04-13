#!/usr/bin/env python

from SCons.Script import ARGUMENTS, Environment, Mkdir, Default, File, CacheDir, SConscript
import os
import sys

# Project configuration
PROJECT_NAME = "dtl"

# Resolve platform/target/arch from args
platform = ARGUMENTS.get('platform')
if not platform:
    p = sys.platform
    if p.startswith('win'):
        platform = 'windows'
    elif p == 'darwin':
        platform = 'macos'
    else:
        platform = 'linux'

target = ARGUMENTS.get('target', 'template_release')
arch = ARGUMENTS.get('arch', 'universal' if platform == 'macos' else 'x86_64')
if arch not in ['x86_64', 'x86_32', 'arm64', 'universal']:
    print(f"ERROR: Invalid architecture '{arch}'. Supported architectures are: x86_64, x86_32, arm64, universal.")
    Exit(1)

# Set up the environment based on the platform
use_mingw_arg = ARGUMENTS.get('use_mingw', 'no')
use_mingw = use_mingw_arg.lower() in ['yes', 'true', '1']

if platform == 'windows' and not use_mingw:
    env = Environment(tools=['default', 'msvc'])
elif platform == 'windows' and use_mingw:
    env = Environment(tools=['gcc', 'g++', 'gnulink', 'ar', 'gas'])
    cc_cmd = os.environ.get('CC', 'x86_64-w64-mingw32-gcc')
    cxx_cmd = os.environ.get('CXX', 'x86_64-w64-mingw32-g++')
    env.Replace(CC=cc_cmd)
    env.Replace(CXX=cxx_cmd)
    env.Replace(LINK=cxx_cmd)
else:
    env = Environment()

# Optional: enable SCons cache
cache_dir = os.environ.get('SCONS_CACHE_DIR')
if cache_dir:
    CacheDir(cache_dir)

# Add include paths
env.Append(CPPPATH=[
    'src',
    'include/godot-cpp/include',
    'include/godot-cpp/gen/include',
    'include/godot-cpp/gdextension',
    'include/DungeonTemplateLibrary/include',
])

# Platform-specific compiler flags
is_windows = platform == 'windows'

if is_windows and not use_mingw:
    env.Append(CCFLAGS=['/std:c++17', '/EHsc'])
    if target == 'template_debug':
        env.Append(CCFLAGS=['/Od', '/MDd', '/Zi'])
        env.Append(LINKFLAGS=['/DEBUG'])
    else:
        env.Append(CCFLAGS=['/O2', '/MD'])
elif is_windows and use_mingw:
    env.Append(CCFLAGS=['-std=c++17'])
    env.Append(CCFLAGS=['-g', '-O0'] if target == 'template_debug' else ['-O3'])
    env.Append(CCFLAGS=['--sysroot=/usr/x86_64-w64-mingw32'])
    env.Append(LINKFLAGS=['--sysroot=/usr/x86_64-w64-mingw32'])
else:
    env.Append(CCFLAGS=['-std=c++17'])
    env.Append(CCFLAGS=['-g', '-O0'] if target == 'template_debug' else ['-O3'])

# Link against precompiled godot-cpp
env.Append(LIBPATH=['include/godot-cpp/bin'])

lib_prefix = "lib"
if is_windows and not use_mingw:
    lib_prefix = ""
    lib_ext = ".lib"
else:
    lib_ext = ".a"

# Handle macOS universal builds
if platform == 'macos' and arch == 'universal':
    possible_libs = [
        f"{lib_prefix}godot-cpp.{platform}.{target}.{arch}{lib_ext}",
        f"{lib_prefix}godot-cpp.{platform}.{target}.x86_64{lib_ext}",
        f"{lib_prefix}godot-cpp.{platform}.{target}.arm64{lib_ext}",
    ]
    godot_cpp_lib = None
    for lib in possible_libs:
        if os.path.exists(os.path.join('include', 'godot-cpp', 'bin', lib)):
            godot_cpp_lib = lib
            break
    if not godot_cpp_lib:
        print(f"ERROR: Could not find godot-cpp library. Tried:")
        for lib in possible_libs:
            print(f"  - {os.path.join('include', 'godot-cpp', 'bin', lib)}")
        Exit(1)
elif platform == 'macos':
    possible_libs = [
        f"{lib_prefix}godot-cpp.{platform}.{target}.{arch}{lib_ext}",
        f"{lib_prefix}godot-cpp.{platform}.{target}.universal{lib_ext}",
    ]
    godot_cpp_lib = None
    for lib in possible_libs:
        if os.path.exists(os.path.join('include', 'godot-cpp', 'bin', lib)):
            godot_cpp_lib = lib
            break
    if not godot_cpp_lib:
        godot_cpp_lib = possible_libs[0]
else:
    godot_cpp_lib = f"{lib_prefix}godot-cpp.{platform}.{target}.{arch}{lib_ext}"

env.Append(LIBS=[File(os.path.join('include', 'godot-cpp', 'bin', godot_cpp_lib))])

# Build godot-cpp only if pre-built library is missing
godot_cpp_lib_path = os.path.join('include', 'godot-cpp', 'bin', godot_cpp_lib)
if os.path.exists(godot_cpp_lib_path):
    print(f"Using pre-built godot-cpp: {godot_cpp_lib_path}")
else:
    print("Building godot-cpp (pre-built library not found)...")
    SConscript("include/godot-cpp/SConstruct")

# Platform-specific system libraries
if is_windows:
    env.Append(LIBS=['kernel32', 'user32', 'gdi32', 'winspool', 'comdlg32', 'advapi32',
                     'shell32', 'ole32', 'oleaut32', 'uuid', 'odbc32', 'odbccp32'])
elif platform == 'linux':
    env.Append(LIBS=['pthread', 'dl'])
elif platform == 'macos':
    env.Append(LIBS=['pthread'])
    env.Append(FRAMEWORKS=['CoreFoundation'])

# Source files
sources = Glob("src/*.cpp")

# Debug logging
print("=== SCons build configuration ===")
print("Project:", PROJECT_NAME)
print("platform:", platform)
print("target:", target)
print("arch:", arch)
print("expected godot_cpp_lib:", godot_cpp_lib)
print("env['CC']:", env.get('CC'))
print("env['CXX']:", env.get('CXX'))
print("==================================")

# Build the shared library with output matching the .gdextension paths
if platform == "macos":
    # macOS frameworks require the executable name to match the framework name
    # e.g. dtl.macos.template_release.framework/dtl.macos.template_release
    # We must set SHLIBPREFIX to empty to prevent SCons adding "lib" prefix
    framework_name = "dtl.{}.{}".format(platform, target)
    env['SHLIBPREFIX'] = ''
    env['SHLIBSUFFIX'] = ''
    library = env.SharedLibrary(
        "addons/dtl/bin/{}.framework/{}".format(framework_name, framework_name),
        source=sources,
    )
elif platform == "ios":
    if arch == "simulator":
        library = env.StaticLibrary(
            "addons/dtl/bin/dtl.{}.{}.simulator.a".format(platform, target),
            source=sources,
        )
    else:
        library = env.StaticLibrary(
            "addons/dtl/bin/dtl.{}.{}.a".format(platform, target),
            source=sources,
        )
else:
    # Construct suffix to match gdextension naming: dtl.platform.target.arch.ext
    if is_windows:
        ext = ".dll"
    else:
        ext = ".so"
    library = env.SharedLibrary(
        "addons/dtl/bin/dtl.{}.{}.{}{}".format(platform, target, arch, ext),
        source=sources,
    )

Default(library)
