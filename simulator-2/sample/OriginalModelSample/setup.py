#-*-coding:utf-8-*-
import sys
import os
from setuptools.command.install import install as install_orig
from distutils.command.build import build as build_orig
from setuptools import setup,find_packages
import glob

packageName="OriginalModelSample"
version="1.0.0"
build_config="Debug" if "--Debug" in sys.argv else "Release"
i=0
while i<len(sys.argv):
	arg=sys.argv[i]
	if(arg=="--Debug" or arg=="--Release"):
		sys.argv.pop(i)
	else:
		i+=1
isMSYS=False

class build(build_orig):
	def run(self):
		import sys
		import subprocess
		import numpy
		import pybind11
		import ASRCAISim1
		prefix=sys.base_prefix
		py_ver="%d.%d" % sys.version_info[:2]
		if(os.name=="nt"):
			python_include_dir=os.path.dirname(glob.glob(os.path.join(prefix,"include/Python.h"))[0])
			python_lib_dir=os.path.join(prefix,"libs")
		else:
			python_include_dir=os.path.dirname(glob.glob(os.path.join(prefix,"include/python"+py_ver+sys.abiflags+"/Python.h"))[0])
			python_lib_dir=os.path.join(prefix,"lib")
		numpy_include_dir=numpy.get_include()
		pybind11_cmake_dir=pybind11.get_cmake_dir()
		core_simulator_dir=os.path.abspath(os.path.dirname(ASRCAISim1.__file__))
		if(os.name=="nt"):
			if(isMSYS):
				subprocess.check_call([".\\builder_MSYS.bat",
					build_config,
					python_include_dir.replace(os.path.sep,'/'),
					python_lib_dir.replace(os.path.sep,'/'),
					numpy_include_dir.replace(os.path.sep,'/'),
					pybind11_cmake_dir.replace(os.path.sep,'/'),
            		core_simulator_dir.replace(os.path.sep,'/')
				])
			else:
				subprocess.check_call([".\\builder.bat",
					build_config,
					python_include_dir.replace(os.path.sep,'/'),
					python_lib_dir.replace(os.path.sep,'/'),
					numpy_include_dir.replace(os.path.sep,'/'),
					pybind11_cmake_dir.replace(os.path.sep,'/'),
            		core_simulator_dir.replace(os.path.sep,'/')
				])
			dummy=os.path.join(os.path.dirname(__file__),packageName+"/lib"+packageName+".pyd")
			if(os.path.exists(dummy)):
				os.remove(dummy)
			f=open(dummy,"w") #Dummy
			f.close()
		else:
			subprocess.check_call(["bash","./builder.sh",
				build_config,
				python_include_dir,
				python_lib_dir,
				numpy_include_dir,
				pybind11_cmake_dir,
            	core_simulator_dir
			])
		build_orig.run(self)

class install(install_orig):
	user_options = install_orig.user_options + [
		("MSYS",None,"whether use MSYS or not")
	]
	
	def initialize_options(self):
		install_orig.initialize_options(self)
		self.MSYS = None
	def finalize_options(self):
		install_orig.finalize_options(self)
		global isMSYS
		isMSYS = os.name=="nt" and self.MSYS is not None
	def run(self):
		install_orig.run(self)
		if(os.name=="nt"):
			dummy=os.path.join(os.path.dirname(__file__),packageName+"/lib"+packageName+".pyd")
			if(os.path.exists(dummy)):
				os.remove(dummy) #Dummy

requirements=[]
headers=[os.path.relpath(p,"./"+packageName) for p in glob.glob(packageName+"/include/**",recursive=True)]
configs=[os.path.relpath(p,"./"+packageName) for p in glob.glob(packageName+"/config/*.json")]

manifest=open("MANIFEST.in","w")
with open("MANIFEST.in.self","r") as fragment:
	manifest.write(fragment.read()+"\n")
manifest.close()

setup(
	name=packageName,
	version=version,
	author="Air Systems Research Center, ATLA",
	packages=find_packages(),
	include_package_data=True,
	cmdclass={"build":build,"install":install},
	setup_requires=["numpy","pybind11>=2.6.2"]
)