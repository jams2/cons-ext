from setuptools import Extension, setup

setup(
    ext_modules=[
        Extension(
            name="fastcons",
            sources=["consmodule.c"],
            headers=["consmodule.h"],
        )
    ],
    package_data={"fastcons": ["fastcons.pyi"]},
)
