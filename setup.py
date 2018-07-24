from setuptools import setup, find_packages
from distutils.dir_util import copy_tree
import glob
import os
import subprocess
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
#
# Board specific package delivery setup
#
# excludes file from path and returns other files
def exclude_from_files(exclude, path):
    return [file for file in os.listdir(path)
            if os.path.isfile(os.path.join(path, file))
            and file != exclude]


def exclude_from_dirs(exclude, path):
    return [folder for folder in os.listdir(path)
            if os.path.isdir(os.path.join(path, folder))
            and folder != exclude]

# find dirs containing .bit files
def find_designs(path):
    return [f for f in os.listdir(path)
            if os.path.isdir(os.path.join(path, f))
            and len(glob.glob(os.path.join(path, f, "*.bit"))) > 0]

# collect and package the board's overlay designs
def collect_ekf_designs():
    design_files = []
    design_dirs = find_designs(board_folder)
    print("dirs: ", design_dirs)
    for ds in design_dirs:
        print("ds: ", ds)
        print(os.getcwd())
        print(os.listdir(os.getcwd()))
        copy_tree(os.path.join(board_folder, ds), os.path.join("ekf", ds))
        #print(os.listdir(os.path.join(os.getcwd(), "ekf/full")))
        new_dir = os.path.join("ekf", ds) # location for .bit .tcl .so files
        f1 = open(os.path.join(new_dir, "__init__.py"), "w")
        f1.close()
        print("nd: ", new_dir)
        files = exclude_from_files("makefile", new_dir) # example to exclude
        print("f: ", files)
        design_files.extend([os.path.join(new_dir, f) for f in files])
    print("dfs: ", design_files)
    #print(adaf)
    return design_files

# build shared object using makefile
def build_shared_object(board):
    
    arch = "arm-linux-gnueabihf-g++"
    if board in ["Ultra96", "ZCU104"]:
        arch = "aarch64-linux-gnu-g++"

    build_folder = "build/dist/{}/".format(board)
    try:
        locs = [x[0] for x in os.walk(build_folder) if len(x[0].split(
            "/"))==5]
    except Exception:
        raise BoardSupportError("%s does not exist" %build_folder)
    names = [x.split("/")[-1] for x in locs]
    if len(names) == 0:
        return

    # build all shared objects
    for i, loc in enumerate(locs):
        bf = "/".join(loc.split("/")[-3:-1])
        print(bf)
        cmd = ["make", "install", "BOARD=%s"%(bf), "NAME=%s"%(names[i]),
               "CC=%s"%(arch), "-C", "./build"]
        status = subprocess.check_call(cmd)
        if status is not 0:
            print("Error while running make... exiting")
            sys.exit(1)

#1. Build the shared object
build_shared_object(board)

#2. Copy ekf design files to ekf package
ekf_data_files = []
ekf_data_files.extend(collect_ekf_designs())

#3. Copy notebooks
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
    #data_files=ekf_data_files,
    include_package_data=True,
    description="Extended Kalman Filter (EKF) supporting multiple PYNQ "
                "enabled boards"
)
