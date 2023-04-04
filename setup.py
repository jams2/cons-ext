from distutils.core import setup, Extension


def main():
    setup(
        name="cons",
        version="0.1.0",
        ext_modules=[Extension("cons", ["consmodule.c"])],
    )


if __name__ == "__main__":
    main()
