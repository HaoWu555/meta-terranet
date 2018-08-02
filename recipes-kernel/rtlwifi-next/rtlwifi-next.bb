SUMMARY = "Build terranet wifi driver into Linux kernel module"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://LICENSE;md5=a23a74b3f4caf9616230789d94217acb"

DEPENDS = "virtual/kernel"
inherit module  

#KERNEL_MODULE_AUTOLOAD += "rtlwifi-next"

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI= "git://${BSPDIR}/sources/terranet_support/rtlwifi-next;protocol=file;branch=${SRCBRANCH}\
          file://0001-modify-the-KSCR-variable-in-Makefile.patch\
		  file://0002-make-Makefile-fit-yocto.patch"

SRCBRANCH = "master"
SRCREV = "3f6a961c42950a859dc1f25d9c31394e9931ca6a"

S = "${WORKDIR}/git"
B = "${WORKDIR}/git"
#D = "${WORKDIR}/image"

EXTRA_OEMAKE += "KSRC=${STAGING_KERNEL_DIR} \
			     MODDESTDIR=${STAGING_KERNEL_DIR}/drivers/net/wireless/rtlwifi"

#FILES_${PN} += "${libexecdir} /lib/modules/${KERNEL_VERSION}/rtlwifi-next"
INHIBIT_PACKAGE_STRIP = "1"

do_install_append () {
	install -d ${D}/lib/firmware/rtlwifi/ 
	install -m 755 ${B}/firmware/rtlwifi/rtl8822befw.bin ${D}/lib/firmware/rtlwifi/rtl8822befw.bin
}
FILES_${PN}= "/lib/firmware/rtlwifi/rtl8822befw.bin"

# The inherit of module.bbclass will automatically name module packages with
# "kernel-module-" prefix as required by the oe-core build environment.
#Recipes that rely on the kernel source code and do not inherit the module classes might need to add explicit dependencies on the do_shared_workdir kernel task:

