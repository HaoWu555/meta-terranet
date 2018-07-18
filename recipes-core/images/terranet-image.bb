# Base this image on core-image-base
include recipes-core/images/core-image-base.bb

DESCRIPTION = "Image for TerraNet demoboard"

# Include modules in rootfs
IMAGE_INSTALL += " \
    kernel-modules \
    "

IMAGE_FEATURES += "ssh-server-dropbear splash"



