#!/usr/bin/env bash

SCRIPT_DIR=$(readlink -f $(dirname $0))
ASTERISK_ROOT_DIR=$(cd ${SCRIPT_DIR}/../../../ && pwd)

__ISA_BITS="64"
_LIBDIR="/usr/lib64"
__GLOBAL_LDFLAGS="-Wl,-z,relro -Wl,-z,now"
_DATADIR="/usr/share"
_LOCALSTATEDIR="/var"

OPT_FLAGS="-DLUA_COMPAT_MODULE -fPIC -Ofast"
LD_FLAGS="-m${__ISA_BITS} -Wl,--as-needed,--library-path=${_LIBDIR} ${__GLOBAL_LDFLAGS}"
AST_VAR_RUN_DIR="/run/asterisk"
MAKE_ARGS="DEBUG= OPTIMIZE= ASTVARRUNDIR=${AST_VAR_RUN_DIR} ASTDATADIR=${_DATADIR}/asterisk ASTVARLIBDIR=${_DATADIR}/asterisk ASTDBDIR=${_LOCALSTATEDIR}/spool/asterisk NOISY_BUILD=1"

export CFLAGS="${OPT_FLAGS}"
export CXXFLAGS="${OPT_FLAGS}"
export FFLAGS="${OPT_FLAGS}"
export LDFLAGS="${LD_FLAGS}"
export ASTCFLAGS=" "

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
patch --strip=1 --forward --input=${ASTERISK_ROOT_DIR}/contrib/ives/patches/0001-ives_asterisk-18.12.1-explicit-python3.patch
patch --strip=1 --forward --input=${ASTERISK_ROOT_DIR}/contrib/ives/patches/0002-commu_asterisk-mariadb.patch
patch --strip=1 --forward --input=${ASTERISK_ROOT_DIR}/contrib/ives/patches/0003-ives_mysql-devel_to_mariadb-devel.patch



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
