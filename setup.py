from distutils.core import setup, Extension


setup(name='Python posix rwlock',
      version='0.0.1',
      description='Posix rwlock wrapper class',
      ext_modules=[
          Extension('posix_rwlock', sources=['posix_rwlock.c'])
      ])
