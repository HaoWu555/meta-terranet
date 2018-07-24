SUMMARY = "Build terranet wifi driver into Linux kernel module"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://LICENSE;md5=a23a74b3f4caf9616230789d94217acb"

inherit module setuptools

FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI= "file://LICENSE\
		　file://Makefile\
"


S = "${WORKDIR}/git/rtlwifi-terranet"

# The inherit of module.bbclass will automatically name module packages with
# "kernel-module-" prefix as required by the oe-core build environment.
#Recipes that rely on the kernel source code and do not inherit the module classes might need to add explicit dependencies on the do_shared_workdir kernel task:

#do_configure[depends] += "virtual/kernel:do_shared_workdir"

