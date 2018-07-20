# Released under the MIT license (see COPYING.MIT for the terms)

#SUMMARY = "Linux kernel"

require recipes-kernel/linux/linux-imx.inc

#DEPENDS += "lzop-native bc-native"

SRC_URI = "git://${BSPDIR}/sources/linux-imx;protocol=file\
           file://defconfig"
SRCBRANCH = "master"
SRCREV = "f15ea3fe065e11c3a6f3153e99d4c08af23b421b"
LINUX_VERSION ?= "4.9.11"
#LINUX_VERSION_EXTENSION ??= "-terranet-${LINUX_KERNEL_TYPE}"

# We need to pass it as param since kernel might support more then one
# machine, with different entry points
#KERNEL_EXTRA_ARGS += "LOADADDR=${UBOOT_ENTRYPOINT}"
