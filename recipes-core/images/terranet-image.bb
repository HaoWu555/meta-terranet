# Base this image on core-image-base
#include ${BSPDIR}/sources/poky/meta/recipes-core/images/core-image-base.bb
include recipes-core/images/core-image-minimal.bb

DESCRIPTION = "Image for TerraNet demoboard"

# Include modules in rootfs
IMAGE_INSTALL += " \
    kernel-modules \
	rtlwifi-next \
	iw \
	pciutils \
	wireless-tools \
	linux-firmware-ath6k\
	linux-firmware-imx-sdma-imx6q\
	firmware-imx-vpu-imx6q\
	vim\
	empty-root-password\
	"
IMAGE_FEATURES += "splash package-management ssh-server-dropbear hwcodecs"

