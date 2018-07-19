#!/bin/bash

export KBUILD_OUTPUT=../imx6_tn/out

export INSTALL_PATH=../imx6_tn/deploy

export INSTALL_MOD_PATH=../imx6_tn/deploy

export PATH=$PATH:~/terranet/buildroot/output/host/bin/
export TOOLCHAIN=~/terranet/buildroot/output/host/
export CROSS_COMPILE=arm-buildroot-linux-uclibcgnueabihf-
export ARCH=arm
