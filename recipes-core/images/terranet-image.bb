# this image is based on core-image-base
#include ${BSPDIR}/sources/poky/meta/recipes-core/images/core-image-base.bb
include recipes-core/images/core-image-base.bb

DESCRIPTION = "Image for TerraNet demoboard"

# Include modules in rootfs
IMAGE_INSTALL_append = " \
    kernel-modules \
	rtlwifi-next \
	pciutils \
	vim\
	glibc-staticdev \
	packagegroup-base-wifi \
	"
IMAGE_FEATURES += "splash package-management ssh-server-dropbear hwcodecs empty-root-password"

DISTRO_FEATURES += "ipv6 wifi bluetooth systemd ${DISTRO_FEATURES_LIBC}"
