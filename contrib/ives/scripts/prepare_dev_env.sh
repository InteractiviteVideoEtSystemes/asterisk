#!/usr/bin/env bash

SCRIPT_DIR=$(readlink -f $(dirname $0))
ASTERISK_ROOT_DIR=$(cd ${SCRIPT_DIR}/../../../ && pwd)



#
#   INSTALL BUILD & RUN DEPENDANCIES
#
yum-builddep -y ${ASTERISK_ROOT_DIR}/contrib/ives/files/packaging/asterisk.spec
REQUIRES_LIST=$(grep ^Requires ${ASTERISK_ROOT_DIR}/contrib/ives/files/packaging/asterisk.spec | sed 's/ \+/ /g' | cut -d' ' -f2 | tr '\n' ' ')
dnf install -y ${REQUIRES_LIST}



#
#   PATCHES
#
# /!\ Order matters /!\
#
cd ${ASTERISK_ROOT_DIR}
patch --strip=1 --forward --input=${ASTERISK_ROOT_DIR}/contrib/ives/patches/0002-commu_asterisk-mariadb.patch



#
#   INSTALL DEPENDENCIES AND SOURCES
#
${ASTERISK_ROOT_DIR}/contrib/scripts/install_prereq install
${ASTERISK_ROOT_DIR}/contrib/scripts/get_mp3_source.sh
${ASTERISK_ROOT_DIR}/bootstrap.sh



#
#   INSTALL SYSTEM FILES
#
mkdir -p /run/asterisk/
cp -a ${ASTERISK_ROOT_DIR}/contrib/ives/files/packaging/asterisk-logrotate /etc/logrotate.d/asterisk
cp -a ${ASTERISK_ROOT_DIR}/contrib/ives/files/packaging/asterisk-service /usr/lib/systemd/system/asterisk.service
cp -a ${ASTERISK_ROOT_DIR}/contrib/ives/files/packaging/asterisk-tmpfiles /usr/lib/tmpfiles.d/asterisk.conf
