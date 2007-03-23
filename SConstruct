# -*- python -*-
#
# Setup our environment
#
import glob
import lsst.SConsUtils as scons

env = scons.makeEnv("fw",
                    r"$HeadURL$",
                    [["boost", "boost/version.hpp", "boost_filesystem:C++"],
                     ["vw", "vw/Core.h", "vw:C++"],
                     ["python", "Python.h"],
		     ["cfitsio", "fitsio.h", "m cfitsio", "ffopen"],
                     ["wcstools", "wcs.h", "wcs", "wcscat"],
                     ["xpa", "xpa.h", "xpa", "XPAPuts"],
                     ])
#
# Libraries that I need to link things.  This should be handled better
#
env.libs = dict([
    ("boost",	Split("boost_filesystem")),
    ("fits",	Split("fitsio")),
    ("vw",	Split("vw vwCore vwFileIO")),
    ])
#
# Build/install things
#
for d in Split("doc examples include/lsst lib src tests") + \
        Split("python/lsst/fw/Catalog python/lsst/fw/Display"):
    SConscript("%s/SConscript" % d)

env['IgnoreFiles'] = r"(~$|\.pyc$|^\.svn$|\.o$)"

Alias("install", env.Install(env['prefix'], "python"))
Alias("install", env.Install(env['prefix'], "include"))
Alias("install", env.Install(env['prefix'], "lib"))
Alias("install", env.Install(env['prefix'] + "/bin", glob.glob("bin/*.py")))
Alias("install", env.InstallEups(env['prefix'] + "/ups", glob.glob("ups/*.table")))

scons.CleanTree(r"*~ core *.so *.os *.o")

env.Declare()
env.Help("""
LSST FrameWork packages
""")

