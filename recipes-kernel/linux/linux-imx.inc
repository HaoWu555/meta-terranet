# Copyright (C) 2012, 2015 O.S. Systems Software LTDA.
# Released under the MIT license (see COPYING.MIT for the terms)

LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=d7810fab7487fb0aad327b76f1be7cd7"

inherit kernel fsl-kernel-localversion fsl-vivante-kernel-driver-handler

# Put a local version until we have a true SRCREV to point to
LOCALVERSION ?= "-1.0.0"
SCMVERSION ?= "y"
SRCBRANCH ?= "imx_4.9.11_1.0.0_ga"
SRCREV = "c27010d99a3d91703ea2d1a3f9630a9dedc3f86f"
DEPENDS += "lzop-native bc-native"
COMPATIBLE_MACHINE = "(mx6|mx7)"
DEFAULT_PREFERENCE = "1"

SRC_URI = "git://source.codeaurora.org/external/imx/linux-imx;protocol=https;branch=${SRCBRANCH} \
           file://defconfig \
"

S = "${WORKDIR}/git"

# We need to pass it as param since kernel might support more then one
# machine, with different entry points
KERNEL_EXTRA_ARGS += "LOADADDR=${UBOOT_ENTRYPOINT}"
