from setuptools import setup, Extension


setup(name="Python posix rwlock",
      version="0.0.1",
      description="Posix rwlock wrapper class",

      author="Zaitsev Kirill",
      author_email="k.zaitsev@me.com",
      url="https://github.com/teferi/python_posix_rwlock",

      packages=["posix_rwlock"],
      ext_modules=[
          Extension("posix_rwlock", sources=["posix_rwlock.c"])
      ])
