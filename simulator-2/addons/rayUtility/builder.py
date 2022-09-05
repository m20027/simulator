def run(build,core_simulator_dir,addon_dst_dir,addon_include_dir,addon_name,isMSYS):
    import sys
    import os
    import glob
    import shutil
    if(os.path.exists(addon_dst_dir)):
        shutil.rmtree(addon_dst_dir)
    if(os.path.exists(addon_include_dir)):
        shutil.rmtree(addon_include_dir)
    pyscripts=glob.glob(os.path.join(os.path.dirname(__file__),"**/*.py"),recursive=True)
    for src in pyscripts:
        dst=os.path.join(addon_dst_dir,os.path.relpath(src,os.path.dirname(__file__)))
        os.makedirs(os.path.dirname(dst),exist_ok=True)
        shutil.copyfile(src,dst)