# Released under the MIT license (see COPYING.MIT for the terms)

SUMMARY = "Linux kernel"

#inherit kernel 
require recipes-kernel/linux/linux-fslc.inc
#require ${BSPDIR}/sources/meta-freescale/recipes-kernel/linux/linux-imx.inc

DEPENDS += "lzop-native bc-native"

SRC_URI = "git://${BSPDIR}/sources/linux-imx;protocol=file\
           file://defconfig"
SRCBRANCH = "master"
SRCREV = "c0afd16bbba673d05e8c6b8c951e2743bc85ee14"
#SRCREV = "f15ea3fe065e11c3a6f3153e99d4c08af23b421b"
#LICENSE = "GPLv2"
#LIC_FILES_CHKSUM = "file://COPYING;md5=838c366f69b72c5df05c96dff79b35f2"

#LINUX_VERSION_EXTENSION ??= "-terranet-${LINUX_KERNEL_TYPE}"
#LOCALVERSION = "-terranet"

# We need to pass it as param since kernel might support more then one
# machine, with different entry points
KERNEL_EXTRA_ARGS += "LOADADDR=${UBOOT_ENTRYPOINT}"
