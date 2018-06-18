from setuptools import setup, find_packages
import os
import subprocess
import site
import sys
import shutil
import ekf


class BoardSupportError(Exception):
    def __init__(self, message):
        self.message = message


# Check that repository supports the board
board = os.environ['BOARD']
board_folder = 'boards/{}/'.format(board)
if not os.path.isdir(board_folder):
    raise BoardSupportError("Repository does not support the %s board" %board)

# Board specific package delivery setup
def exclude_from_files(exclude, path):
    return [file for file in os.listdir(path)
            if os.path.isfile(os.path.join(path, file))
            and file != exclude]


def exclude_from_dirs(exclude, path):
    return [folder for folder in os.listdir(path)
            if os.path.isdir(os.path.join(path, folder))
            and folder != exclude]


def collect_ekf_data_files():
    return [(os.path.join(
        '{}/ekf'.format(os.path.dirname(site.__file__) \
             + "/site-packages"), ol),
             [os.path.join(board_folder, ol, f)
              for f in exclude_from_files(
                 'makefile', os.path.join(board_folder, ol))])
        for ol in exclude_from_dirs('notebooks', board_folder)]


def build_shared_object(board):
    cmd = ["make", "install", "BOARD=%s"%(board), "-C", "./build"]
    status = subprocess.check_call(cmd)
    if status is not 0:
        print("Error while running make... exiting")
        sys.exit(1)



# Build the shared object
build_shared_object(board)

# Copy notebooks
notebook_dir = os.environ['PYNQ_JUPYTER_NOTEBOOKS']
notebook_folder = os.path.join(notebook_dir, "ekf")
source_folder = "notebooks/"
if os.path.exists(notebook_folder):
    shutil.rmtree(notebook_folder)
shutil.copytree(source_folder, notebook_folder)


setup(
    name="pynq-ekf",
    version=ekf.__version__,
    url='https://github.com/sfox14/pynq-ekf',
    license='BSD 3-Clause License',
    author="Sean Fox",
    author_email="sean.fox@sydney.edu.au",
    packages=find_packages(), #['ekf'],
    package_data={'' : ['*.bit','*.tcl','*.so']},
    data_files=collect_ekf_data_files(),
    description="Extended Kalman Filter (EKF) supporting multiple PYNQ "
                "enabled boards"
)
