SUMMARY = "Build terranet wifi driver into Linux kernel module"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://LICENSE;md5=a23a74b3f4caf9616230789d94217acb"

DEPENDS = "virtual/kernel"
inherit module  

#KERNEL_MODULE_AUTOLOAD += "rtlwifi-next"

FILESEXTRAPATHS_prepend := "${THISDIR}/file:"

SRC_URI= "git://${BSPDIR}/sources/terranet_support/rtlwifi-next;protocol=file;branch=${SRCBRANCH}"

SRCBRANCH = "master"
#SRCREV = "b18813dd5e6df80861927bcca85753c867641bac"
SRCREV = "053c1e763e0aede7318f9c1be9e327e58c1aa27a"

S = "${WORKDIR}/git"
B = "${WORKDIR}/git"
#D = "${WORKDIR}/Image"

#FILES_${PN} += "${libexecdir} /lib/modules/${KERNEL_VERSION}/rtlwifi-next"
INHIBIT_PACKAGE_STRIP = "1"

# The inherit of module.bbclass will automatically name module packages with
# "kernel-module-" prefix as required by the oe-core build environment.
#Recipes that rely on the kernel source code and do not inherit the module classes might need to add explicit dependencies on the do_shared_workdir kernel task:

#do_configure[depends] += "virtual/kernel:do_shared_workdir"

