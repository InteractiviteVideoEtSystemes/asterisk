# HOW TO PREPARE YOUR DEVELOPMENT ENVIRONMENT

We are starting from AlmaLinux 8 ISO, the most updated AlmaLinux-8.*-x86_64-minimal.iso

## Prepare your system

### Install repositories

We have to install and activate some repositories:

    dnf install -y epel-release
    dnf config-manager --set-enabled powertools ha

### Update your OS

    dnf update -y

### Install build packages

    dnf install -y git rpm-build rpmdevtools yum-utils

## Prepare your build environment

Fetch source code

    mkdir -p ~/rpmbuild/SOURCES/
    git clone --branch="ives/18.12.1" git@github.com:InteractiviteVideoEtSystemes/asterisk.git ~/rpmbuild/SOURCES/asterisk/

Run the dedicated script

    ~/rpmbuild/SOURCES/asterisk/contrib/ives/scripts/prepare_dev_env.sh
