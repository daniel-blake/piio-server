#!/usr/bin/python

import os,sys
import glob
import shutil
import errno
from subprocess import call
import version


# Config
DEBFULLNAME="Miqra Engineering Packaging"
DEBEMAIL="packaging@miqra.nl"



# Code
os.environ['DEBFULLNAME'] = DEBFULLNAME
os.environ['DEBEMAIL'] = DEBEMAIL

## Debian package building
DEBBUILDDIR="deb_dist"
DEBSOURCEPKG="{0}_{1}.orig.tar.gz".format(version.PACKAGE, version.VERSION)
TARBALLNAME="{0}-{1}.tar.gz".format(version.PACKAGE, version.VERSION)
DEBSOURCEDIR=os.path.join(DEBBUILDDIR,"{0}-{1}".format(version.PACKAGE, version.VERSION))
PKGDIR="{0}-{1}".format(version.PACKAGE, version.VERSION)

PKGPATH=os.path.join(DEBBUILDDIR,PKGDIR)

CWD = os.getcwd()

try:
    os.makedirs(DEBSOURCEDIR)
except OSError as exc: # Python >2.5
    if exc.errno == errno.EEXIST and os.path.isdir(DEBSOURCEDIR):
        pass
    else: raise

call(["make","dist"])

shutil.move(TARBALLNAME , os.path.join(DEBBUILDDIR,DEBSOURCEPKG))

os.chdir(DEBBUILDDIR)
call(["tar","-xzvf",DEBSOURCEPKG])

print "Entering dir " + PKGDIR
os.chdir(PKGDIR)
print "Now in ", os.getcwd()
call(["dh_make --single -yes --copyright bsd"],shell=True)

for f in glob.glob(os.path.join(CWD,"debian","*")):
    dst = os.path.join(CWD,PKGPATH,"debian",os.path.basename(f))
    shutil.copy2(f,dst)

call("debuild",shell=True)

