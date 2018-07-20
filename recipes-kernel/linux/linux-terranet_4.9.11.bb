# Released under the MIT license (see COPYING.MIT for the terms)

#inherit kernel
#require recipes-kernel/linux/linux-terranet.inc
require recipes-kernel/linux/linux-terranet.inc

COMPATIBLE_MACHINE = "(mx6)"
COMPATIBLE_MACHINE_imx6qdlsabresd = "imx6qdlsabresd"
#KERNEL_SRC = "git://${BSPDIR}/sources/linux-imx;protocol=file"
#SRCBRANCH = "master"
#SRCREV = "f15ea3fe065e11c3a6f3153e99d4c08af23b421b"


# Override SRC_URI in a copy of this recipe to point at a different source
# tree if you do not want to build from Linus' tree.
#SRC_URI = "git://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git;protocol=git;nocheckout=1;name=machine"

#LINUX_VERSION ?= "4.9%"
#LINUX_VERSION_EXTENSION_append = "-custom"


# Modify SRCREV to a different commit hash in a copy of this recipe to
# build a different release of the Linux kernel.
# tag: v4.2 64291f7db5bd8150a74ad2036f1037e6a0428df2
#SRCREV_machine="64291f7db5bd8150a74ad2036f1037e6a0428df2"

#PV = "${LINUX_VERSION}+git${SRCPV}"

