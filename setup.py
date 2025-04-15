#!/usr/bin/env python

# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.
#

import sys
import os
import sysconfig
import io

<<<<<<< HEAD
__version__ = "0.9.2"
||||||| 3697152
__version__ = '0.9.2'
=======
from pybind11.setup_helpers import Pybind11Extension, ParallelCompile
from setuptools import setup

ParallelCompile().install()

__version__ = '0.9.2.4'
>>>>>>> 93501d1069ecb00758137c19bb27a53ee097a0e4
FASTTEXT_SRC = "src"

<<<<<<< HEAD
# Based on https://github.com/pybind/python_example


class get_pybind_include:
    """Helper class to determine the pybind11 include path

    The purpose of this class is to postpone importing pybind11
    until it is actually installed, so that the ``get_include()``
    method can be invoked."""

    def __init__(self, user=False):
        try:
            pass
        except ImportError:
            if subprocess.call([sys.executable, "-m", "pip", "install", "pybind11"]):
                raise RuntimeError("pybind11 install failed.")

        self.user = user

    def __str__(self):
        import pybind11

        return pybind11.get_include(self.user)


try:
    coverage_index = sys.argv.index("--coverage")
except ValueError:
    coverage = False
else:
    del sys.argv[coverage_index]
    coverage = True
||||||| 3697152
# Based on https://github.com/pybind/python_example

class get_pybind_include(object):
    """Helper class to determine the pybind11 include path

    The purpose of this class is to postpone importing pybind11
    until it is actually installed, so that the ``get_include()``
    method can be invoked. """

    def __init__(self, user=False):
        try:
            import pybind11
        except ImportError:
            if subprocess.call([sys.executable, '-m', 'pip', 'install', 'pybind11']):
                raise RuntimeError('pybind11 install failed.')

        self.user = user

    def __str__(self):
        import pybind11
        return pybind11.get_include(self.user)

try:
    coverage_index = sys.argv.index('--coverage')
except ValueError:
    coverage = False
else:
    del sys.argv[coverage_index]
    coverage = True
=======
WIN = sys.platform.startswith("win32") and "mingw" not in sysconfig.get_platform()
>>>>>>> 93501d1069ecb00758137c19bb27a53ee097a0e4

fasttext_src_files = map(str, os.listdir(FASTTEXT_SRC))
fasttext_src_cc = list(filter(lambda x: x.endswith(".cc"), fasttext_src_files))

fasttext_src_cc = list(
    map(lambda x: str(os.path.join(FASTTEXT_SRC, x)), fasttext_src_cc)
)

<<<<<<< HEAD
ext_modules = [
    Extension(
        str("fasttext_pybind"),
        [
            str("python/fasttext_module/fasttext/pybind/fasttext_pybind.cc"),
        ]
        + fasttext_src_cc,
        include_dirs=[
            # Path to pybind11 headers
            get_pybind_include(),
            get_pybind_include(user=True),
            # Path to fasttext source code
            FASTTEXT_SRC,
        ],
        language="c++",
        extra_compile_args=[
            "-O0 -fno-inline -fprofile-arcs -pthread -march=native"
            if coverage
            else "-O3 -funroll-loops -pthread -march=native"
        ],
    ),
]


# As of Python 3.6, CCompiler has a `has_flag` method.
# cf http://bugs.python.org/issue26689
def has_flag(compiler, flags):
    """Return a boolean indicating whether a flag name is supported on
    the specified compiler.
    """
    import tempfile

    with tempfile.NamedTemporaryFile("w", suffix=".cpp") as f:
        f.write("int main (int argc, char **argv) { return 0; }")
        try:
            compiler.compile([f.name], extra_postargs=flags)
        except setuptools.distutils.errors.CompileError:
            return False
    return True


def cpp_flag(compiler):
    """Return the -std=c++17 compiler flag."""
    standards = ["-std=c++17"]
    for standard in standards:
        if has_flag(compiler, [standard]):
            return standard
    raise RuntimeError("Unsupported compiler -- at least C++17 support " "is needed!")


class BuildExt(build_ext):
    """A custom build extension for adding compiler-specific options."""

    c_opts = {
        "msvc": ["/EHsc"],
        "unix": [],
    }

    def build_extensions(self):
        if sys.platform == "darwin":
            mac_osx_version = float(".".join(platform.mac_ver()[0].split(".")[:2]))
            os.environ["MACOSX_DEPLOYMENT_TARGET"] = str(mac_osx_version)
            all_flags = ["-stdlib=libc++", "-mmacosx-version-min=10.7"]
            if has_flag(self.compiler, [all_flags[0]]):
                self.c_opts["unix"] += [all_flags[0]]
            elif has_flag(self.compiler, all_flags):
                self.c_opts["unix"] += all_flags
            else:
                raise RuntimeError(
                    "libc++ is needed! Failed to compile with {} and {}.".format(
                        " ".join(all_flags), all_flags[0]
                    )
                )
        ct = self.compiler.compiler_type
        opts = self.c_opts.get(ct, [])
        extra_link_args = []

        if coverage:
            coverage_option = "--coverage"
            opts.append(coverage_option)
            extra_link_args.append(coverage_option)

        if ct == "unix":
            opts.append('-DVERSION_INFO="%s"' % self.distribution.get_version())
            opts.append(cpp_flag(self.compiler))
            if has_flag(self.compiler, ["-fvisibility=hidden"]):
                opts.append("-fvisibility=hidden")
        elif ct == "msvc":
            opts.append('/DVERSION_INFO=\\"%s\\"' % self.distribution.get_version())
        for ext in self.extensions:
            ext.extra_compile_args = opts
            ext.extra_link_args = extra_link_args
        build_ext.build_extensions(self)
||||||| 3697152
ext_modules = [
    Extension(
        str('fasttext_pybind'),
        [
            str('python/fasttext_module/fasttext/pybind/fasttext_pybind.cc'),
        ] + fasttext_src_cc,
        include_dirs=[
            # Path to pybind11 headers
            get_pybind_include(),
            get_pybind_include(user=True),
            # Path to fasttext source code
            FASTTEXT_SRC,
        ],
        language='c++',
        extra_compile_args=["-O0 -fno-inline -fprofile-arcs -pthread -march=native" if coverage else
                            "-O3 -funroll-loops -pthread -march=native"],
    ),
]


# As of Python 3.6, CCompiler has a `has_flag` method.
# cf http://bugs.python.org/issue26689
def has_flag(compiler, flags):
    """Return a boolean indicating whether a flag name is supported on
    the specified compiler.
    """
    import tempfile
    with tempfile.NamedTemporaryFile('w', suffix='.cpp') as f:
        f.write('int main (int argc, char **argv) { return 0; }')
        try:
            compiler.compile([f.name], extra_postargs=flags)
        except setuptools.distutils.errors.CompileError:
            return False
    return True


def cpp_flag(compiler):
    """Return the -std=c++[11/14] compiler flag.
    The c++14 is preferred over c++11 (when it is available).
    """
    standards = ['-std=c++11']
    for standard in standards:
        if has_flag(compiler, [standard]):
            return standard
    raise RuntimeError(
        'Unsupported compiler -- at least C++11 support '
        'is needed!'
    )


class BuildExt(build_ext):
    """A custom build extension for adding compiler-specific options."""
    c_opts = {
        'msvc': ['/EHsc'],
        'unix': [],
    }

    def build_extensions(self):
        if sys.platform == 'darwin':
            mac_osx_version = float('.'.join(platform.mac_ver()[0].split('.')[:2]))
            os.environ['MACOSX_DEPLOYMENT_TARGET'] = str(mac_osx_version)
            all_flags = ['-stdlib=libc++', '-mmacosx-version-min=10.7']
            if has_flag(self.compiler, [all_flags[0]]):
                self.c_opts['unix'] += [all_flags[0]]
            elif has_flag(self.compiler, all_flags):
                self.c_opts['unix'] += all_flags
            else:
                raise RuntimeError(
                    'libc++ is needed! Failed to compile with {} and {}.'.
                    format(" ".join(all_flags), all_flags[0])
                )
        ct = self.compiler.compiler_type
        opts = self.c_opts.get(ct, [])
        extra_link_args = []

        if coverage:
            coverage_option = '--coverage'
            opts.append(coverage_option)
            extra_link_args.append(coverage_option)

        if ct == 'unix':
            opts.append('-DVERSION_INFO="%s"' % self.distribution.get_version())
            opts.append(cpp_flag(self.compiler))
            if has_flag(self.compiler, ['-fvisibility=hidden']):
                opts.append('-fvisibility=hidden')
        elif ct == 'msvc':
            opts.append(
                '/DVERSION_INFO=\\"%s\\"' % self.distribution.get_version()
            )
        for ext in self.extensions:
            ext.extra_compile_args = opts
            ext.extra_link_args = extra_link_args
        build_ext.build_extensions(self)
=======
extra_compile_args = []
if WIN:
    extra_compile_args.append('/DVERSION_INFO=\\"%s\\"' % __version__)
else:
    extra_compile_args.append('-DVERSION_INFO="%s"' % __version__)
    extra_compile_args.extend(["-O3", "-flto"])
>>>>>>> 93501d1069ecb00758137c19bb27a53ee097a0e4


def _get_readme():
    """
    Use pandoc to generate rst from md.
    pandoc --from=markdown --to=rst --output=python/README.rst python/README.md
    """
<<<<<<< HEAD
    with io.open("python/README.rst", encoding="utf-8") as fid:
||||||| 3697152
    with io.open("python/README.rst", encoding='utf-8') as fid:
=======
    with io.open("README.rst", encoding='utf-8') as fid:
>>>>>>> 93501d1069ecb00758137c19bb27a53ee097a0e4
        return fid.read()


setup(
<<<<<<< HEAD
    name="fasttext",
||||||| 3697152
    name='fasttext',
=======
    name='fasttext-predict',
>>>>>>> 93501d1069ecb00758137c19bb27a53ee097a0e4
    version=__version__,
<<<<<<< HEAD
    author="Onur Celebi",
    author_email="celebio@fb.com",
    description="fasttext Python bindings",
||||||| 3697152
    author='Onur Celebi',
    author_email='celebio@fb.com',
    description='fasttext Python bindings',
=======
    author='Alexandre Flament',
    author_email='alex.andre@al-f.net',
    keywords=['fasttext', 'language detection', 'language identification'],
    description='fasttext with wheels and no external dependency, but only the predict method (<1MB)',
>>>>>>> 93501d1069ecb00758137c19bb27a53ee097a0e4
    long_description=_get_readme(),
<<<<<<< HEAD
    ext_modules=ext_modules,
    url="https://github.com/facebookresearch/fastText",
    license="MIT",
||||||| 3697152
    ext_modules=ext_modules,
    url='https://github.com/facebookresearch/fastText',
    license='MIT',
=======
    url='https://github.com/searxng/fasttext-predict/',
    license='MIT',
>>>>>>> 93501d1069ecb00758137c19bb27a53ee097a0e4
    classifiers=[
<<<<<<< HEAD
        "Development Status :: 3 - Alpha",
        "Intended Audience :: Developers",
        "Intended Audience :: Science/Research",
        "License :: OSI Approved :: MIT License",
        "Programming Language :: Python :: 2.7",
        "Programming Language :: Python :: 3.4",
        "Programming Language :: Python :: 3.5",
        "Programming Language :: Python :: 3.6",
        "Topic :: Software Development",
        "Topic :: Scientific/Engineering",
        "Operating System :: Microsoft :: Windows",
        "Operating System :: POSIX",
        "Operating System :: Unix",
        "Operating System :: MacOS",
||||||| 3697152
        'Development Status :: 3 - Alpha',
        'Intended Audience :: Developers',
        'Intended Audience :: Science/Research',
        'License :: OSI Approved :: MIT License',
        'Programming Language :: Python :: 2.7',
        'Programming Language :: Python :: 3.4',
        'Programming Language :: Python :: 3.5',
        'Programming Language :: Python :: 3.6',
        'Topic :: Software Development',
        'Topic :: Scientific/Engineering',
        'Operating System :: Microsoft :: Windows',
        'Operating System :: POSIX',
        'Operating System :: Unix',
        'Operating System :: MacOS',
=======
        'Development Status :: 3 - Alpha',
        'Intended Audience :: Developers',
        'Intended Audience :: Science/Research',
        'License :: OSI Approved :: MIT License',
        'Programming Language :: Python :: 3.9',
        'Programming Language :: Python :: 3.10',
        'Programming Language :: Python :: 3.11',
        'Programming Language :: Python :: 3.12',
        'Programming Language :: Python :: 3.13',
        'Topic :: Software Development',
        'Topic :: Scientific/Engineering',
        'Operating System :: Microsoft :: Windows',
        'Operating System :: POSIX',
        'Operating System :: Unix',
        'Operating System :: MacOS',
>>>>>>> 93501d1069ecb00758137c19bb27a53ee097a0e4
    ],
<<<<<<< HEAD
    install_requires=["pybind11>=2.2", "setuptools >= 0.7.0", "numpy"],
    cmdclass={"build_ext": BuildExt},
||||||| 3697152
    install_requires=['pybind11>=2.2', "setuptools >= 0.7.0", "numpy"],
    cmdclass={'build_ext': BuildExt},
=======
>>>>>>> 93501d1069ecb00758137c19bb27a53ee097a0e4
    packages=[
<<<<<<< HEAD
        str("fasttext"),
        str("fasttext.util"),
        str("fasttext.tests"),
||||||| 3697152
        str('fasttext'),
        str('fasttext.util'),
        str('fasttext.tests'),
=======
        'fasttext',
>>>>>>> 93501d1069ecb00758137c19bb27a53ee097a0e4
    ],
<<<<<<< HEAD
    package_dir={str(""): str("python/fasttext_module")},
||||||| 3697152
    package_dir={str(''): str('python/fasttext_module')},
=======
    package_dir={
        '': 'python/fasttext_module'
    },
>>>>>>> 93501d1069ecb00758137c19bb27a53ee097a0e4
    zip_safe=False,
    ext_modules=[
        Pybind11Extension(
            "fasttext_pybind",
            ["python/fasttext_module/fasttext/pybind/fasttext_pybind.cc"] + fasttext_src_cc,
            include_dirs=[
                FASTTEXT_SRC,
            ],
            cxx_std=17,
            extra_compile_args=extra_compile_args,
        )
    ],
)
