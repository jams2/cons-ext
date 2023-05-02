import os
import sysconfig

from setuptools import Extension, setup

DEBUG = int(os.environ.get("CONS_DEBUG", 0)) == 1
DEBUG_LEVEL = 0

# Common flags for both release and debug builds.
extra_compile_args = sysconfig.get_config_var("CFLAGS").split()
extra_compile_args += [
    "-Wall",
    "-Wextra",
    "-Wno-unused-parameter",
    "-Wconversion",
    "-Wsign-conversion",
    "--std=c11",
]
if DEBUG:
    extra_compile_args += [
        "-g3",
        "-O0",
        "-DDEBUG=%s" % DEBUG_LEVEL,
        "-UNDEBUG",
        "-fanalyzer",
    ]
else:
    extra_compile_args += ["-DNDEBUG", "-O3"]


setup(
    ext_modules=[
        Extension(
            name="fastcons",
            sources=["consmodule.c"],
            extra_compile_args=extra_compile_args,
        )
    ]
)
