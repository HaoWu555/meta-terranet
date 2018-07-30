# Base this image on core-image-base
#include ${BSPDIR}/sources/poky/meta/recipes-core/images/core-image-base.bb
require recipes-core/images/core-image-minimal.bb

DESCRIPTION = "Image for TerraNet demoboard"

# Include modules in rootfs
IMAGE_INSTALL += " \
    kernel-modules \
	rtlwifi-next  \
    "

IMAGE_FEATURES += "splash package-management ssh-server-dropbear hwcodecs"


