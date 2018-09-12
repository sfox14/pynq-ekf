from setuptools import setup, find_packages
from distutils.dir_util import copy_tree
import glob
import os
import subprocess
import sys
import shutil


# global variables
board = os.environ['BOARD']
board_folder = 'boards/{}/'.format(board)
notebooks_dir = os.environ['PYNQ_JUPYTER_NOTEBOOKS']
ekf_data_files = []


# check whether board is supported
def check_env():
    if not os.path.isdir(board_folder):
        raise ValueError("Board {} is not supported.".format(board))
    if not os.path.isdir(notebooks_dir):
        raise ValueError("Directory {} does not exist.".format(notebooks_dir))


# excludes file from path and returns other files
def exclude_from_files(exclude, path):
    return [file for file in os.listdir(path)
            if os.path.isfile(os.path.join(path, file))
            and file != exclude]


# find dirs containing .bit files
def find_designs(path):
    return [f for f in os.listdir(path)
            if os.path.isdir(os.path.join(path, f))
            and len(glob.glob(os.path.join(path, f, "*.bit"))) > 0]


# collect and package the board's overlay designs
def collect_ekf_designs():
    design_dirs = find_designs(board_folder)
    for ds in design_dirs:
        new_dir = os.path.join("ekf", ds)
        copy_tree(os.path.join(board_folder, ds), new_dir)
        files = exclude_from_files("makefile", new_dir)
        ekf_data_files.extend([os.path.join("..", new_dir, f) for f in files])


# Copy notebooks in boards/BOARD/notebooks
def copy_notebooks():
    if os.path.isdir(board_folder):
        dst_folder = os.path.join(notebooks_dir, 'ekf')
        if os.path.exists(dst_folder):
            shutil.rmtree(dst_folder)
        copy_tree(os.path.join(board_folder, 'notebooks', 'ekf'), dst_folder)


check_env()
collect_ekf_designs()
copy_notebooks()


setup(
    name="pynq-ekf",
    version='1.0',
    install_requires=[
          'rig',
          'pynq>=2.3'
    ],
    url='https://github.com/sfox14/pynq-ekf',
    license='BSD 3-Clause License',
    author="Sean Fox",
    author_email="sean.fox@sydney.edu.au",
    packages=find_packages(),
    package_data={
        '': ekf_data_files,
    },
    description="Extended Kalman Filter (EKF) supporting PYNQ-enabled boards"
)
