#-*-coding:utf-8-*-
import sys
import os
from setuptools.command.install import install as install_orig
from distutils.command.build import build as build_orig
from setuptools import setup,find_packages
import glob

packageName="ASRCAISim1"
build_config="Debug" if "--Debug" in sys.argv else "Release"
include_addons=["rayUtility","AgentIsolation"]
exclude_addons=[]
include_samples=["config","Standard","OriginalModelSample","MinimumEvaluation"]
i=0
while i<len(sys.argv):
	arg=sys.argv[i]
	if(arg.startswith("--addons")):
		include_addons.extend(arg[9:].split(","))
		sys.argv.pop(i)
	elif(arg.startswith("--ex-addons")):
		exclude_addons.extend(arg[12:].split(","))
		sys.argv.pop(i)
	elif(arg=="--Debug" or arg=="--Release"):
		sys.argv.pop(i)
	else:
		i+=1
exclude_addons=list(set(exclude_addons))
include_addons=list(set(include_addons))
i=0
while i<len(include_addons):
	if(include_addons[i] in exclude_addons):
		include_addons.pop(i)
	else:
		i+=1
isMSYS=False

class build(build_orig):
	def run(self):
		import sys
		import subprocess
		import numpy
		import pybind11
		prefix=sys.base_prefix
		py_ver="%d.%d" % sys.version_info[:2]
		if(os.name=="nt"):
			python_include_dir=os.path.dirname(glob.glob(os.path.join(prefix,"include","Python.h"))[0])
			python_lib_dir=os.path.join(prefix,"libs")
		else:
			python_include_dir=os.path.dirname(glob.glob(os.path.join(prefix,"include/python"+py_ver+sys.abiflags+"/Python.h"))[0])
			python_lib_dir=os.path.join(prefix,"lib")
		numpy_include_dir=numpy.get_include()
		pybind11_cmake_dir=pybind11.get_cmake_dir()
		if(os.name=="nt"):
			if(isMSYS):
				subprocess.check_call([".\\builder_MSYS.bat",
					build_config,
					python_include_dir.replace(os.path.sep,'/'),
					python_lib_dir.replace(os.path.sep,'/'),
					numpy_include_dir.replace(os.path.sep,'/'),
					pybind11_cmake_dir.replace(os.path.sep,'/')
				])
			else:
				subprocess.check_call([".\\builder.bat",
					build_config,
					python_include_dir.replace(os.path.sep,'/'),
					python_lib_dir.replace(os.path.sep,'/'),
					numpy_include_dir.replace(os.path.sep,'/'),
					pybind11_cmake_dir.replace(os.path.sep,'/')
				])
			dummy=os.path.join(os.path.dirname(__file__),packageName,"libCore.pyd")
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
				pybind11_cmake_dir
			])
		core_simulator_dir=os.path.abspath(os.path.join(os.path.dirname(__file__),packageName))
		addon_base_dir=os.path.abspath(os.path.join(os.path.dirname(__file__),"addons"))
		cwd=os.getcwd()
		for addon in include_addons:
			addon_src_dir=os.path.join(addon_base_dir,addon)
			os.chdir(addon_src_dir)
			addon_dst_dir=os.path.join(core_simulator_dir,"addons",addon)
			addon_include_dir=os.path.join(core_simulator_dir,"include",packageName,"addons",addon)
			import importlib
			spec=importlib.util.spec_from_file_location("builder","builder.py")
			builder=importlib.util.module_from_spec(spec)
			spec.loader.exec_module(builder)
			builder.run(
				build_config,
				core_simulator_dir.replace(os.path.sep,'/'),
				addon_dst_dir.replace(os.path.sep,'/'),
				addon_include_dir.replace(os.path.sep,'/'),
				addon,
				isMSYS
			)
			del builder
			os.chdir(cwd)
			if(os.name=="nt"):
				if(os.path.exists(os.path.join(addon_dst_dir,"lib"+addon+".dll")) or
					os.path.join(addon_dst_dir,addon+".dll")):
					dummy=os.path.join(addon_dst_dir,"lib"+addon+".pyd")
					if(os.path.exists(dummy)):
						os.remove(dummy)
					f=open(dummy,"w") #Dummy
					f.close()
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
			dummy=os.path.join(os.path.dirname(__file__),packageName+"/libCore.pyd")
			if(os.path.exists(dummy)):
				os.remove(dummy) #Dummy
			core_simulator_dir=os.path.join(os.path.dirname(__file__),packageName)
			for addon in include_addons:
				dummy=os.path.join(core_simulator_dir,"addons/"+addon,"lib"+addon+".pyd")
				if(os.path.exists(dummy)):
					os.remove(dummy) #Dummy

version=open("version.txt").read().strip()
requirements=open("requirements.txt","r").read().splitlines()
headers=glob.glob(packageName+"/include/"+packageName+"/**",recursive=True)+\
	glob.glob(packageName+"/include/thirdParty/**",recursive=True)
configs=glob.glob(packageName+"/config/*.json")
extra_requires={}
manifest=open("MANIFEST.in","w")
with open("MANIFEST.in.fragment","r") as fragment:
	manifest.write(fragment.read()+"\n")
for sample in include_samples:
	with open("sample/"+sample+"/MANIFEST.in.fragment","r") as fragment:
		manifest.write(fragment.read()+"\n")
core_simulator_dir=os.path.abspath(os.path.join(os.path.dirname(__file__),packageName))
addon_base_dir=os.path.abspath(os.path.join(os.path.dirname(__file__),"addons"))
for addon in include_addons:
	addon_src_dir=os.path.join(addon_base_dir,addon)
	addon_dst_dir=os.path.join(core_simulator_dir,"addons/"+addon)
	addon_include_dir=os.path.join(core_simulator_dir,"include/"+packageName+"/addons/"+addon)
	with open(os.path.join(addon_src_dir,"MANIFEST.in.fragment"),"r") as fragment:
		manifest.write(fragment.read()+"\n")
	if os.path.exists(os.path.join(addon_src_dir,"requirements.txt")):
		extra_requires[addon]=open(os.path.join(addon_src_dir,"requirements.txt"),"r").read().splitlines()
manifest.close()

setup(
	name=packageName,
	version=version,
	author="Air Systems Research Center, ATLA",
	packages=find_packages(),
	include_package_data=True,
	cmdclass={"build":build,"install":install},
	setup_requires=["numpy","pybind11>=2.6.2"],
	install_requires=requirements,
	license=("license.txt"),
	extras_require=extra_requires
)