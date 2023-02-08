#!/usr/bin/env python
import shutil
from os.path import dirname, join, realpath

from setuptools import find_packages, setup

ROOT_FOLDER = dirname(realpath(__file__))
VERSION_FILE_PATH = join(ROOT_FOLDER, 'src', '_version.py')

try:
    from karabo.packaging.versioning import device_scm_version
    scm_version = device_scm_version(ROOT_FOLDER, VERSION_FILE_PATH)
except ImportError:
    # compatibility with karabo versions earlier than 2.10
    scm_version = {'write_to': VERSION_FILE_PATH}


setup(name='DsscControl',
      use_scm_version=scm_version,
      author='Detector Group',
      author_email='dssc',
      description='',
      long_description='',
      url='',
      package_dir={'': 'src'},
      packages=find_packages('src'),
      entry_points={
          'karabo.middlelayer_device': [
              'DsscControl = DsscControl.dssc_control:DsscControl',
              'DsscASICreset = DsscControl.DsscASICreset:DsscASICreset',
              'DsscConfigSetter = DsscControl.dsscConfSetter:DsscConfigSetter'
          ],
          'karabo.bound_device': [
              'DsscSIB = DsscSIB.DsscSIB:DsscSIB',
          ],
      },
      package_data={},
      requires=[],
      )


# copy to subpaths with Karabo class files

shutil.copy(VERSION_FILE_PATH,
            join(ROOT_FOLDER, "src/DsscControl"))
shutil.copy(VERSION_FILE_PATH,
            join(ROOT_FOLDER, "src/DsscSIB"))
