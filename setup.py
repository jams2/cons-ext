from setuptools import Extension, setup

setup(
    ext_modules=[
        Extension(
            name="fastcons",
            sources=["consmodule.c"],
        )
    ]
)
