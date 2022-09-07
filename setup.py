from skbuild import setup
from setuptools import find_packages

setup(
    packages=find_packages('cython', exclude=["test"]),
    package_dir={"": "cython"},
    cmake_languages=["C"],
    install_requires=["sounddevice"],
)
