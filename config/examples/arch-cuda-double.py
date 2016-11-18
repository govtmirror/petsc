#!/usr/bin/python
#
if __name__ == '__main__':
  import sys
  import os
  sys.path.insert(0, os.path.abspath('config'))
  import configure
  configure_options = [
    '--with-cuda=1',
    '--with-cusp=1',
    '-with-cusp-dir=/home/balay/soft/cusplibrary-0.4.0',
    '--with-thrust=1',
    '--with-precision=double',
    '--with-clanguage=c',
    '--with-cuda-arch=sm_20'

  ]
  configure.petsc_configure(configure_options)
