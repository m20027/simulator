def run(build,core_simulator_dir,addon_dst_dir,addon_include_dir,addon_name,isMSYS):
    import sys
    import os
    import glob
    import shutil
    import subprocess
    import numpy
    import pybind11
    print("core_simulator_dir=",core_simulator_dir)
    if(os.path.exists(addon_dst_dir)):
        shutil.rmtree(addon_dst_dir)
    if(os.path.exists(addon_include_dir)):
        shutil.rmtree(addon_include_dir)
    prefix=sys.base_prefix
    py_ver="%d.%d" % sys.version_info[:2]
    if(os.name=="nt"):
        python_include_dir=os.path.dirname(glob.glob(os.path.join(prefix,"include/Python.h"))[0]).replace(os.path.sep,'/')
        python_lib_dir=os.path.join(prefix,"libs").replace(os.path.sep,'/')
    else:
        python_include_dir=os.path.dirname(glob.glob(os.path.join(prefix,"include/python"+py_ver+sys.abiflags+"/Python.h"))[0])
        python_lib_dir=os.path.join(prefix,"lib")
    numpy_include_dir=numpy.get_include()
    pybind11_cmake_dir=pybind11.get_cmake_dir()
    if(os.name=="nt"):
        if(isMSYS):
            libname="python"+"%d%d" % sys.version_info[:2]
            subprocess.check_call([".\\builder_MSYS.bat",
                build,
                python_include_dir,
                python_lib_dir,
                numpy_include_dir,
                pybind11_cmake_dir,
                core_simulator_dir,
                addon_dst_dir,
                addon_include_dir,
                addon_name
            ])
        else:
            subprocess.check_call([".\\builder.bat",
                build,
                python_include_dir,
                python_lib_dir,
                numpy_include_dir,
                pybind11_cmake_dir,
                core_simulator_dir,
                addon_dst_dir,
                addon_include_dir,
                addon_name
            ])
    else:
        subprocess.check_call(["bash","./builder.sh",
            build,
            python_include_dir,
            python_lib_dir,
            numpy_include_dir,
            pybind11_cmake_dir,
            core_simulator_dir,
            addon_dst_dir,
            addon_include_dir,
            addon_name
        ])
def package_data(addon_name):
    packageName=addon_name
    return ["lib"+packageName+".so",
                packageName+".lib",
                packageName+".dll",
                "lib"+packageName+".dll.a",
                "lib"+packageName+".dll",
                "lib"+packageName+".pyd"]