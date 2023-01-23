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
#   BUILD ASTERISK
#
cd ${ASTERISK_ROOT_DIR}
${ASTERISK_ROOT_DIR}/configure --prefix=/usr --libdir=${_LIBDIR} --with-pjproject-bundled
# MAKE_ARGS = 'DEBUG= OPTIMIZE= ASTVARRUNDIR=/run/asterisk ASTDATADIR=/usr/share/asterisk ASTVARLIBDIR=/usr/share/asterisk ASTDBDIR=/var/spool/asterisk NOISY_BUILD=1'
make ${MAKE_ARGS}
make menuselect.makeopts ${MAKE_ARGS}

# Enable Add-ons used by IVèS
menuselect/menuselect --enable chan_mobile --enable chan_ooh323 --enable format_mp3 --enable res_config_mysql menuselect.makeopts
# Disable deprecated applications
menuselect/menuselect --disable app_adsiprog --disable app_getcpeid menuselect.makeopts
# Enable codec translators used by IVèS
menuselect/menuselect --enable codec_opus --enable codec_silk --enable codec_siren7 --enable codec_siren14 --enable codec_g729a menuselect.makeopts
# Disable deprecated resource modules
menuselect/menuselect --disable res_adsi --disable res_monitor menuselect.makeopts
# Enable resource modules used by IVèS
menuselect/menuselect --enable res_chan_stats --enable res_endpoint_stats menuselect.makeopts
# Disable deprecated resource modules
menuselect/menuselect --disable res_adsi --disable res_pktccops menuselect.makeopts
# Enable core sound packages used by IVèS
menuselect/menuselect --enable CORE-SOUNDS-EN-SLN16 --enable CORE-SOUNDS-EN-WAV --enable CORE-SOUNDS-EN_AU-SLN16 --enable CORE-SOUNDS-EN_AU-WAV --enable CORE-SOUNDS-EN_GB-SLN16 --enable CORE-SOUNDS-EN_GB-WAV --enable CORE-SOUNDS-ES-G722 --enable CORE-SOUNDS-ES-SLN16 --enable CORE-SOUNDS-ES-WAV --enable CORE-SOUNDS-FR-SLN16 --enable CORE-SOUNDS-FR-WAV --enable CORE-SOUNDS-IT-SLN16 --enable CORE-SOUNDS-IT-WAV --enable CORE-SOUNDS-JA-SLN16 --enable CORE-SOUNDS-JA-WAV --enable CORE-SOUNDS-RU-SLN16 --enable CORE-SOUNDS-RU-WAV --enable CORE-SOUNDS-SV-SLN16 --enable CORE-SOUNDS-SV-WAV menuselect.makeopts
# Enable music on hold file packages used by IVèS
menuselect/menuselect --enable MOH-OPSOUND-SLN16 --enable MOH-OPSOUND-WAV menuselect.makeopts
# Enable extras sound packages used by IVèS
menuselect/menuselect --enable EXTRA-SOUNDS-EN-SLN16 --enable EXTRA-SOUNDS-EN-WAV --enable EXTRA-SOUNDS-EN_GB-SLN16 --enable EXTRA-SOUNDS-EN_GB-WAV --enable EXTRA-SOUNDS-FR-SLN16 --enable EXTRA-SOUNDS-FR-WAV menuselect.makeopts
# Compiler flags
menuselect/menuselect --enable DONT_OPTIMIZE --disable BUILD_NATIVE menuselect.makeopts

make install ${MAKE_ARGS}
make samples ${MAKE_ARGS}
patch -p1 -N -i ${ASTERISK_ROOT_DIR}/contrib/ives/patches/0004-ives_fix_makefile.patch
make config ${MAKE_ARGS}
make progdocs ${MAKE_ARGS}