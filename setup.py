import os
import sysconfig

from setuptools import Extension, setup

_DEBUG = int(os.environ.get("CONS_DEBUG", 0)) == 1
# Generally I write code so that if DEBUG is defined as 0 then all optimisations
# are off and asserts are enabled. Typically run times of these builds are x2 to x10
# release builds.
# If DEBUG > 0 then extra code paths are introduced such as checking the integrity of
# internal data structures. In this case the performance is by no means comparable
# with release builds.
_DEBUG_LEVEL = 0

# Common flags for both release and debug builds.
extra_compile_args = sysconfig.get_config_var("CFLAGS").split()
extra_compile_args += ["-Wall", "-Wextra", "-pedantic"]
if _DEBUG:
    extra_compile_args += ["-g3", "-O0", "-DDEBUG=%s" % _DEBUG_LEVEL, "-UNDEBUG"]
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
