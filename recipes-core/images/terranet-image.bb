# Base this image on core-image-base
#include ${BSPDIR}/sources/poky/meta/recipes-core/images/core-image-base.bb
include recipes-core/images/core-image-minimal.bb

DESCRIPTION = "Image for TerraNet demoboard"

# Include modules in rootfs
IMAGE_INSTALL += " \
    kernel-modules \
	rtlwifi-next \
	"

IMAGE_FEATURES += "splash package-management ssh-server-dropbear hwcodecs"

IMAGE_FSTYPES = "tar.bz2 ext4"
INHERIT += "buildhistory"

