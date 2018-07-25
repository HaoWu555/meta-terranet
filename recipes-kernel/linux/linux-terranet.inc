# Released under the MIT license (see COPYING.MIT for the terms)

SUMMARY = "Terranet Linux kernel"

LICENSE = "GPLv2"
#LIC_FILES_CHKSUM = "file://COPYING.MIT;md5=838c366f69b72c5df05c96dff79b35f2"
LIC_FILES_CHKSUM = "file://COPYING.MIT;md5=d7810fab7487fb0aad327b76f1be7cd7"

inherit kernel fsl-kernel-localversion fsl-vivante-kernel-driver-handler
#autotools

#FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}-${PV}:" 

SRC_URI = "git://${BSPDIR}/sources/terranet_support/linux-imx;protocol=file;branch=${SRCBRANCH}\
           file://defconfig"

SRCBRANCH = "master"
SRCREV = "f15ea3fe065e11c3a6f3153e99d4c08af23b421b"

KERNEL_SRC = "git://${BSPDIR}/sources/terranet_support/linux-imx;protocol=file;branch=${SRCBRANCH}"

DEPENDS += "lzop-native bc-native"

S = "${WORKDIR}/git"

# We need to pass it as param since kernel might support more then one
# machine, with different entry points
#KERNEL_EXTRA_ARGS += "LOADADDR=${UBOOT_ENTRYPOINT}"

# Put a local version until we have a true SRCREV to point to
LOCALVERSION ?= "-1.0.0"

COMPATIBLE_MACHINE = "(mx6|mx7)"
DEFAULT_PREFERENCE = "1"

