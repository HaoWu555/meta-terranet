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
	"

IMAGE_FEATURES += "splash package-management ssh-server-dropbear hwcodecs"
COMPATIBLE_MACHINE = "(mx6q|mx6dl)"

