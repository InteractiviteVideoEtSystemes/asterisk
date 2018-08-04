%define astapi 13
%define pjprojectversion 2.4

#%define docdir /usr/share/doc/asterisk
%define logdir /var/log
%define _without_misdn 1
%define _without_dahdi 1
%define _without_bluetooth 1
%define _without_resample 1
%define _without_voicemail_odbcstorage 1
%define _without_voicemail_imapstorage 1

Summary: Asterisk, The Open Source PBX
Name: asterisk13
Version: 13.22.1
# reset release to 1 with each version bump
Release: 2%{?dist}
License: GPL
Group: Utilities/System
# Source2: asterisk.logrotate
BuildRoot: %{_tmppath}/asterisk-%{version}-root
URL: http://www.asterisk.org
Packager: Nethesis <info@nethesis.it>
Conflicts: asterisk14
Conflicts: asterisk16
Conflicts: asterisk18
Conflicts: asterisk10
Conflicts: asterisk11
Conflicts: asterisk12
Provides: asterisk%{astapi}
BuildRequires: subversion
Requires: lua
BuildRequires: lua-devel
Requires: portaudio
BuildRequires: portaudio-devel
Requires: neon
BuildRequires: neon-devel
Requires: libxml2
BuildRequires: libxml2-devel
Requires: spandsp
BuildRequires: spandsp-devel
Requires: libical
BuildRequires: libical-devel
Requires: libsrtp
BuildRequires: libsrtp-devel
Requires: freeradius
Requires: radiusclient-ng
BuildRequires: radiusclient-ng-devel
Requires: jack-audio-connection-kit
BuildRequires: jack-audio-connection-kit-devel
Requires: openldap
BuildRequires: openldap-devel
Requires: sqlite
BuildRequires: sqlite-devel
Requires: sqlite2
BuildRequires: sqlite2-devel
Requires: unixODBC
BuildRequires: unixODBC-devel
Requires: libtool-ltdl
BuildRequires: libtool-ltdl-devel

BuildRequires: xmlstarlet wget

%{?_without_optimizations:Requires: %{name}-debuginfo = %{version}-%{release}}
Requires: %{name}-core = %{version}-%{release}
%{!?_without_dahdi:Requires: %{name}-dahdi = %{version}-%{release}}
Requires: %{name}-doc = %{version}
Requires: %{name}-voicemail = %{version}-%{release}
BuildRequires: ncurses-devel
BuildRequires: libxml2-devel
BuildRequires: libuuid-devel
BuildRequires: jansson-devel
%{!?_without_newt:BuildRequires: newt-devel}
BuildConflicts: rh-postgresql-devel

%description
Asterisk is an open source PBX and telephony development platform.  Asterisk
can both replace a conventional PBX and act as a platform for the
development of custom telephony applications for the delivery of dynamic
content over a telephone; similar to how one can deliver dynamic content
through a web browser using CGI and a web server.

Asterisk supports a variety of telephony hardware including BRI, PRI, POTS,
and IP telephony clients using the Inter-Asterisk eXchange (IAX) protocol (e.g.
gnophone or miniphone).  For more information and a current list of supported
hardware, see http://www.asterisk.org

%package core
Summary: Asterisk core package without any "extras".
Group: Utilities/System
Provides: asterisk%{astapi}-core
Conflicts: asterisk14-core
Conflicts: asterisk16-core
Conflicts: asterisk18-core
Conflicts: asterisk10-core
Conflicts: asterisk11-core
Conflicts: asterisk12-core
Requires: openssl
Requires: libxml2
Requires(pre): %{_sbindir}/groupadd
Requires(pre): %{_sbindir}/useradd



%description core
This package contains a base install of Asterisk without any "extras".

#
#  Alsa subpackage
#
%{?_without_alsa:%if 0}
%{!?_without_alsa:%if 1}
%package alsa
Summary: Alsa channel driver for Asterisk
Group: Utilities/System
Provides: asterisk%{astapi}-alsa
#Obsoletes: asterisk-alsa
%if "%{distname}" == "suse" || "%{distname}" == "sles"
BuildRequires: alsa-devel
Requires: alsa
%else
BuildRequires: alsa-lib-devel
Requires: alsa-lib
%endif
Requires: %{name}-core = %{version}-%{release}

%description alsa
Alsa channel driver for Asterisk
%endif

#
# Sound subpackages
#

%package sounds
Summary: Asterisk sound files
Group: Utilities/System

%description sounds
All sound files to be packaged with asterisk

#
#  snmp subpackage
#
%{?_without_snmp:%if 0}
%{!?_without_snmp:%if 1}
%package snmp
Summary: snmp resource module for Asterisk
Group: Utilities/System
Provides: asterisk%{astapi}-snmp
#Obsoletes: asterisk-snmp
%if "%{distname}" == "suse" || "%{distname}" == "sles"
BuildRequires: sensors
BuildRequires: tcpd-devel
%else
BuildRequires: lm_sensors-devel
%endif
BuildRequires: net-snmp-devel
Requires: net-snmp
Requires: %{name}-core = %{version}-%{release}

%description snmp
snmp resource module for Asterisk
%endif

#
#  pgsql subpackage
#
%{?_without_pgsql:%if 0}
%{!?_without_pgsql:%if 1}
%package pgsql
Summary: Postgresql modules for Asterisk
Group: Utilities/System
Provides: asterisk%{astapi}-pgsql
#Obsoletes: asterisk-pgsql
BuildRequires: postgresql-devel
Requires: postgresql
Requires: %{name}-core = %{version}-%{release}

%description pgsql
Postgresql modules for Asterisk
%endif

#
#  tds subpackage
#
%{?_without_tds:%if 0}
%{!?_without_tds:%if 1}
%package tds
Summary: tds modules for Asterisk
Group: Utilities/System
Provides: asterisk%{astapi}-tds
#Obsoletes: asterisk-tds
BuildRequires: freetds-devel
Requires: freetds
Requires: %{name}-core = %{version}-%{release}

%description tds
tds modules for Asterisk
%endif

#
#  dahdi subpackage
#
%{?_without_dahdi:%if 0}
%{!?_without_dahdi:%if 1}
%package dahdi
Summary: DAHDI channel driver for Asterisk
Group: Utilities/System
Provides: asterisk%{astapi}-dahdi
#Obsoletes: asterisk-dahdi
Requires: %{name}-core = %{version}-%{release}
BuildRequires: libopenr2-devel
BuildRequires: libpri-devel
BuildRequires: libss7-devel
BuildRequires: libtonezone-devel
BuildRequires: dahdi-linux-devel
# rem via bryan for pbxtended
#Requires: libopenr2
#Requires: libpri
#Requires: libss7
#Requires: libtonezone
#Requires: dahdi-linux
#Requires: dahdi-linux-kmod
AutoReqProv: no

%description dahdi
DAHDI channel driver for Asterisk
%endif

#
#  mISDN subpackage
#
%{?_without_misdn:%if 0}
%{!?_without_misdn:%if 1}
%package misdn
Summary: mISDN channel driver for Asterisk
Group: Utilities/System
Provides: asterisk%{astapi}-misdn
#Obsoletes: asterisk-misdn
Requires: %{name}-core = %{version}-%{release}
BuildRequires: mISDNuser-devel
BuildRequires: mISDN-devel
Requires: mISDNuser
Requires: mISDN
Requires: mISDN-kmod
AutoReqProv: no

%description misdn
mISDN channel driver for Asterisk
%endif

#
#  Configs subpackage
#
%package configs
Summary: Basic configuration files for Asterisk
Group: Utilities/System
Provides: asterisk%{astapi}-configs
#Obsoletes: asterisk-configs
Requires: %{name}-core = %{version}

%description configs
The sample configuration files for Asterisk

#
#  cURL subpackage
#
%{?_without_curl:%if 0}
%{!?_without_curl:%if 1}
%package curl
Summary: cURL application module for Asterisk
Group: Utilities/System
Provides: asterisk%{astapi}-curl
#Obsoletes: asterisk-curl
BuildRequires: curl-devel
Requires: curl
Requires: %{name}-core = %{version}-%{release}

%description curl
cURL application module for Asterisk
%endif

#
#  Development subpackage
#
%package devel
Summary: Static libraries and header files for Asterisk development
Group: Development/Libraries
Provides: asterisk%{astapi}-devel
#Obsoletes: asterisk-devel
Requires: %{name}-core = %{version}

%description devel
The static libraries and header files needed for building additional Asterisk
plugins/modules

#
#  Documentation subpackage
#
%package doc
Summary: Documentation files for Asterisk
Group: Development/Libraries
Provides: asterisk%{astapi}-doc
#Obsoletes: asterisk-doc
Requires: %{name}-core = %{version}

%description doc
The Documentation files for Asterisk

#
#  Ogg-Vorbis subpackage
#
%{?_without_ogg:%if 0}
%{!?_without_ogg:%if 1}
%package ogg
Summary: Ogg-Vorbis codec module for Asterisk
Group: Utilities/System
Provides: asterisk%{astapi}-ogg
#Obsoletes: asterisk-ogg
BuildRequires: libvorbis-devel libogg-devel
Requires: libvorbis libogg
Requires: %{name}-core = %{version}-%{release}

%description ogg
Asterisk format plugin for the Ogg-Vorbis codec
%endif

#
#  Speex subpackage
#
%{?_without_speex:%if 0}
%{!?_without_speex:%if 1}
%package speex
Summary: Speex codec module for Asterisk
Group: Utilities/System
Provides: asterisk%{astapi}-speex
#Obsoletes: asterisk-speex
BuildRequires: speex-devel
Requires: speex
Requires: %{name}-core = %{version}-%{release}

%description speex
Asterisk format plugin for the Speex codec
%endif

#
#  resample subpackage
#
%{?_without_resample:%if 0}
%{!?_without_resample:%if 1}
%package resample
Summary: resampling codec module for Asterisk
Group: Utilities/System
Provides: asterisk%{astapi}-resample
#Obsoletes: asterisk-resample
BuildRequires: libresample-devel
Requires: libresample
Requires: %{name}-core = %{version}-%{release}

%description resample
Asterisk plugin for the resample codec
%endif

#
#  unixODBC subpackage
#
%{?_without_odbc:%if 0}
%{!?_without_odbc:%if 1}
%package odbc
Summary: Open Database Connectivity (ODBC) drivers for Asterisk
Group: Utilities/System
Provides: asterisk%{astapi}-odbc
#Obsoletes: asterisk-odbc
BuildRequires: unixODBC-devel
Requires: unixODBC
Requires: %{name}-core = %{version}-%{release}

%description odbc
ODBC drivers for Asterisk
%endif

#
#  sqlite3 subpackage
#
%{?_without_sqlite3:%if 0}
%{!?_without_sqlite3:%if 1}
%package sqlite3
Summary: sqlite3 drivers for Asterisk
Group: Utilities/System
Provides: asterisk%{astapi}-sqlite3
#Obsoletes: asterisk-sqlite3
BuildRequires: sqlite-devel
Requires: sqlite
Requires: %{name}-core = %{version}-%{release}

%description sqlite3
sqlite3 drivers for Asterisk
%endif

#
#  voicemail file storage subpackage
#
%package voicemail
Summary: Voicemail with file storage module for Asterisk
Group: Utilities/System
Provides: asterisk%{astapi}-voicemail
Provides: asterisk%{astapi}-voicemail-filestorage
#Obsoletes: asterisk-voicemail
#Obsoletes: asterisk-voicemail-filestorage
Requires: %{name}-core = %{version}-%{release}
Provides: %{name}-voicemail-filestorage = %{version}-%{release}
Conflicts: %{name}-voicemail-odbcstorage
Conflicts: %{name}-voicemail-imapstorage

%description voicemail
Voicemail with file storage module for Asterisk

#
#  voicemail ODBC storage subpackage
#
%{?_without_voicemail_odbcstorage:%if 0}
%{!?_without_voicemail_odbcstorage:%if 1}
%package voicemail-odbcstorage
Summary: Voicemail with ODBC storage module for Asterisk
Group: Utilities/System
Provides: asterisk%{astapi}-voicemail-odbcstorage
#Obsoletes: asterisk-voicemail-odbcstorage
BuildRequires: unixODBC-devel
Requires: unixODBC
Requires: %{name}-core = %{version}-%{release}
Provides: %{name}-voicemail = %{version}-%{release}
Conflicts: %{name}-voicemail-filestorage
Conflicts: %{name}-voicemail-imapstorage

%description voicemail-odbcstorage
Voicemail with ODBC storage module for Asterisk
%endif

#
#  voicemail IMAP storage subpackage
#
%{?_without_voicemail_imapstorage:%if 0}
%{!?_without_voicemail_imapstorage:%if 1}
%package voicemail-imapstorage
Summary: Voicemail with IMAP storage module for Asterisk
Group: Utilities/System
Provides: asterisk%{astapi}-voicemail-imapstorage
#Obsoletes: asterisk-voicemail-imapstorage
%if "%{distname}" == "suse" || "%{distname}" == "sles"
BuildRequires: imap-devel
Requires: imap-lib
%else
BuildRequires: libc-client-devel
Requires: libc-client
%endif
Requires: %{name}-core = %{version}-%{release}
Provides: %{name}-voicemail = %{version}-%{release}
Conflicts: %{name}-voicemail-filestorage
Conflicts: %{name}-voicemail-odbcstorage

%description voicemail-imapstorage
Voicemail with IMAP storage module for Asterisk
%endif

#
#  addons subpackage
#
%package addons
Summary: Asterisk-addons package.
Group: Utilities/System
Provides: asterisk%{astapi}-addons

%{?_without_mysql:%if 0}
%{!?_without_mysql:%if 1}
Requires: %{name}-addons-mysql = %{version}-%{release}
Requires: mysql
%endif

%{?_without_bluetooth:%if 0}
%{!?_without_bluetooth:%if 1}
Requires: %{name}-addons-bluetooth = %{version}-%{release}
Requires: bluez-libs
%endif

%{?_without_ooh323:%if 0}
%{!?_without_ooh323:%if 1}
Requires: %{name}-addons-ooh323 = %{version}-%{release}
%endif

%description addons
This package contains a base install of Asterisk-addons without any "extras".

#
#  addons-core subpackage
#
%package addons-core
Summary: Asterisk-addons core package.
Group: Utilities/System
Requires: asterisk%{astapi}-core = %{version}-%{release}
Provides: asterisk%{astapi}-addons-core

%description addons-core
This package contains a base install of Asterisk-addons without any "extras".

#
#  addons-mysql subpackage
#
%{?_without_mysql:%if 0}
%{!?_without_mysql:%if 1}
%package addons-mysql
Summary: mysql modules for Asterisk
Group: Utilities/System
BuildRequires: mysql-devel
Requires: mysql
Provides: asterisk%{astapi}-addons-mysql

%description addons-mysql
mysql modules for Asterisk
%endif

#
#  bluetooth subpackage
#
%{?_without_bluetooth:%if 0}
%{!?_without_bluetooth:%if 1}
%package addons-bluetooth
Summary: bluetooth modules for Asterisk
Group: Utilities/System
BuildRequires: bluez-libs-devel
Requires: bluez-libs
Requires: %{name}-core = %{version}-%{release}
Provides: asterisk%{astapi}-addons-bluetooth

%description addons-bluetooth
bluetooth modules for Asterisk
%endif

#
#  ooh323 subpackage
#
%{?_without_ooh323:%if 0}
%{!?_without_ooh323:%if 1}
%package addons-ooh323
Summary: chan_ooh323 module for Asterisk
Group: Utilities/System
Requires: %{name}-core = %{version}-%{release}
Provides: asterisk%{astapi}-addons-ooh323

%description addons-ooh323
chan_ooh323 module for Asterisk
%endif

%prep
cd $RPM_SOURCE_DIR/asterisk13
echo %{version} > .version


%build
%ifarch x86_64
%define makeflags OPT=-fPIC
%else
%define makeflags OPT=
%endif
cd $RPM_SOURCE_DIR/%{name}
echo %{version}%{?_without_optimizations:-debug} > .version

# Use Bundled pjproject
./configure --prefix=/usr --libdir=%{_libdir} --with-pjproject-bundled
make menuselect.makeopts
#menuselect/menuselect --list-options to get the options passed below
menuselect/menuselect --disable-category MENUSELECT_CORE_SOUNDS --disable-category MENUSELECT_EXTRA_SOUNDS --disable-category MENUSELECT_MOH --enable-category MENUSELECT_ADDONS --enable res_pktccops --enable chan_mgcp --enable chan_motif --enable app_meetme --enable app_page --enable res_snmp --enable res_srtp --enable DONT_OPTIMIZE --disable BUILD_NATIVE --enable res_statsd --enable res_chan_stats --enable res_endpoint_stats --enable codec_opus --enable codec_silk --enable codec_siren7 --enable codec_siren14 menuselect.makeopts

menuselect/menuselect --enable CORE-SOUNDS-EN-WAV
menuselect/menuselect --enable CORE-SOUNDS-EN-SLN16
menuselect/menuselect --enable CORE-SOUNDS-EN_AU-WAV
menuselect/menuselect --enable CORE-SOUNDS-EN_AU-SLN16
menuselect/menuselect --enable CORE-SOUNDS-EN_GB-WAV
menuselect/menuselect --enable CORE-SOUNDS-EN_GB-SLN16
menuselect/menuselect --enable CORE-SOUNDS-ES-WAV
menuselect/menuselect --enable CORE-SOUNDS-ES-G722
menuselect/menuselect --enable CORE-SOUNDS-ES-SLN16
menuselect/menuselect --enable CORE-SOUNDS-FR-WAV
menuselect/menuselect --enable CORE-SOUNDS-FR-SLN16
menuselect/menuselect --enable CORE-SOUNDS-IT-WAV
menuselect/menuselect --enable CORE-SOUNDS-IT-SLN16
menuselect/menuselect --enable CORE-SOUNDS-RU-WAV
menuselect/menuselect --enable CORE-SOUNDS-RU-SLN16
menuselect/menuselect --enable CORE-SOUNDS-JA-WAV
menuselect/menuselect --enable CORE-SOUNDS-JA-SLN16
menuselect/menuselect --enable CORE-SOUNDS-SV-WAV
menuselect/menuselect --enable CORE-SOUNDS-SV-SLN16
menuselect/menuselect --enable EXTRA-SOUNDS-EN-WAV
menuselect/menuselect --enable EXTRA-SOUNDS-EN-SLN16
menuselect/menuselect --enable EXTRA-SOUNDS-EN_GB-WAV
menuselect/menuselect --enable EXTRA-SOUNDS-EN_GB-SLN16
menuselect/menuselect --enable EXTRA-SOUNDS-FR-WAV
menuselect/menuselect --enable EXTRA-SOUNDS-FR-SLN16
menuselect/menuselect --enable MOH-OPSOUND-WAV
menuselect/menuselect --enable MOH-OPSOUND-SLN16

make %{?_smp_mflags} %{makeflags}

%install
echo DEBUG
mkdir -p $RPM_BUILD_ROOT/home/asterisk/
mkdir -p $RPM_BUILD_ROOT/%{_sysconfdir}/rc.d/init.d/
mkdir -p $RPM_BUILD_ROOT/%{_sysconfdir}/sysconfig/
echo "AST_USER=asterisk" > $RPM_BUILD_ROOT/%{_sysconfdir}/sysconfig/asterisk
echo "AST_GROUP=asterisk" >> $RPM_BUILD_ROOT/%{_sysconfdir}/sysconfig/asterisk
echo "COREDUMP=yes" >> $RPM_BUILD_ROOT/%{_sysconfdir}/sysconfig/asterisk

cd $RPM_SOURCE_DIR/%{name}
make DESTDIR=$RPM_BUILD_ROOT install
make DESTDIR=$RPM_BUILD_ROOT samples
make DESTDIR=$RPM_BUILD_ROOT config

#mkdir -p $RPM_BUILD_ROOT/%{_sysconfdir}/logrotate.d/
#cp %{SOURCE2} $RPM_BUILD_ROOT%{_sysconfdir}/logrotate.d/asterisk
#echo -e "#This comment is to fix rpm file replacing\n#Config file built on $(date)" >> $RPM_BUILD_ROOT%{_sysconfdir}/logrotate.d/asterisk

mkdir -p $RPM_BUILD_ROOT/var/lib/asterisk/licenses/
mkdir -p $RPM_BUILD_ROOT/var/lib/digium/licenses/

# For some reason, when the opus xml doc file is in thirdparty,
# asterisk can't find it and crashes.
mv -f $RPM_BUILD_ROOT/var/lib/asterisk/documentation/thirdparty/*xml $RPM_BUILD_ROOT/var/lib/asterisk/documentation/


%pre core
# Make sure the 'asterisk' user exists
%{_sbindir}/groupadd -r asterisk &>/dev/null || :
%{_sbindir}/useradd -r -s /bin/bash -d /home/asterisk -c 'Asterisk User' -g asterisk asterisk &>/dev/null || :

%post core
ldconfig

# sng7 - asterisk is started by fwconsole
chkconfig --del asterisk || :
systemctl disable asterisk || :

#shmz ha install, so treat it as such and redo the links
if [ -e /etc/schmooze/pbx-ha -a -f /etc/drbd.d/asterisk.res ]; then
	[ ! -h /var/log/asterisk ] && ( rm -rf /var/log/asterisk; ln -s /drbd/asterisk/log /var/log/asterisk )
	[ ! -h /var/spool/asterisk ] && ( rm -rf /var/spool/asterisk ; ln -s /drbd/asterisk/spool /var/spool/asterisk )
	[ ! -h /var/lib/asterisk ] && ( rm -rf /var/lib/asterisk; ln -s /drbd/asterisk/lib /var/lib/asterisk )
	[ ! -h /etc/asterisk ] && ( rm -rf /etc/asterisk; ln -s /drbd/asterisk/etcasterisk /etc/asterisk )
fi

%post
rm -f /etc/logrotate.d/asterisk.rpm*.*

%clean
cd $RPM_BUILD_DIR
%{__rm} -rf asterisk-%{version}
%{__rm} -rf /var/log/%{name}-sources-%{version}-%{release}.make.err
%{__rm} -rf $RPM_BUILD_ROOT

%files
%defattr(-, root, root)

%files core
%defattr(-, root, root)
%config %{_sysconfdir}/rc.d/init.d/asterisk
%config %{_sysconfdir}/sysconfig/asterisk
%attr(0775,asterisk,asterisk) %dir %{_sysconfdir}/asterisk
%attr(0755,asterisk,asterisk) %dir /home/asterisk
%{_libdir}/asterisk/modules/app_adsiprog.so
%{_libdir}/asterisk/modules/app_alarmreceiver.so
%{_libdir}/asterisk/modules/app_amd.so
%{_libdir}/asterisk/modules/app_authenticate.so
%{_libdir}/asterisk/modules/app_cdr.so
%{_libdir}/asterisk/modules/app_celgenuserevent.so
%{_libdir}/asterisk/modules/app_chanisavail.so
%{_libdir}/asterisk/modules/app_channelredirect.so
%{_libdir}/asterisk/modules/app_chanspy.so
%{_libdir}/asterisk/modules/app_confbridge.so
%{_libdir}/asterisk/modules/app_controlplayback.so
%{_libdir}/asterisk/modules/app_db.so
%{_libdir}/asterisk/modules/app_dial.so
%{_libdir}/asterisk/modules/app_dictate.so
%{_libdir}/asterisk/modules/app_directed_pickup.so
%{_libdir}/asterisk/modules/app_directory.so
%{_libdir}/asterisk/modules/app_disa.so
%{_libdir}/asterisk/modules/app_dumpchan.so
%{_libdir}/asterisk/modules/app_echo.so
%{_libdir}/asterisk/modules/app_exec.so
%{_libdir}/asterisk/modules/app_externalivr.so
%{_libdir}/asterisk/modules/app_festival.so
%{_libdir}/asterisk/modules/app_forkcdr.so
%{_libdir}/asterisk/modules/app_followme.so
%{_libdir}/asterisk/modules/app_getcpeid.so
%{_libdir}/asterisk/modules/app_ices.so
%{_libdir}/asterisk/modules/app_image.so
%{_libdir}/asterisk/modules/app_macro.so
%{_libdir}/asterisk/modules/app_milliwatt.so
%{_libdir}/asterisk/modules/app_minivm.so
%{_libdir}/asterisk/modules/app_mixmonitor.so
%{_libdir}/asterisk/modules/app_morsecode.so
%{_libdir}/asterisk/modules/app_mp3.so
%{_libdir}/asterisk/modules/app_nbscat.so
%{_libdir}/asterisk/modules/app_originate.so
%{_libdir}/asterisk/modules/app_page.so
%{_libdir}/asterisk/modules/app_playback.so
%{_libdir}/asterisk/modules/app_playtones.so
%{_libdir}/asterisk/modules/app_privacy.so
%{_libdir}/asterisk/modules/app_queue.so
%{_libdir}/asterisk/modules/app_readexten.so
%{_libdir}/asterisk/modules/app_read.so
%{_libdir}/asterisk/modules/app_record.so
%{_libdir}/asterisk/modules/app_sayunixtime.so
%{_libdir}/asterisk/modules/app_senddtmf.so
%{_libdir}/asterisk/modules/app_sendtext.so
%{_libdir}/asterisk/modules/app_sms.so
%{_libdir}/asterisk/modules/app_softhangup.so
%{_libdir}/asterisk/modules/app_speech_utils.so
%{_libdir}/asterisk/modules/app_stack.so
%{_libdir}/asterisk/modules/app_system.so
%{_libdir}/asterisk/modules/app_talkdetect.so
%{_libdir}/asterisk/modules/app_test.so
%{_libdir}/asterisk/modules/app_transfer.so
%{_libdir}/asterisk/modules/app_url.so
%{_libdir}/asterisk/modules/app_userevent.so
%{_libdir}/asterisk/modules/app_verbose.so
%{_libdir}/asterisk/modules/app_waitforring.so
%{_libdir}/asterisk/modules/app_waitforsilence.so
%{_libdir}/asterisk/modules/app_waituntil.so
%{_libdir}/asterisk/modules/app_while.so
%{_libdir}/asterisk/modules/app_zapateller.so
%{_libdir}/asterisk/modules/bridge_builtin_features.so
%{_libdir}/asterisk/modules/bridge_simple.so
%{_libdir}/asterisk/modules/bridge_softmix.so
%{_libdir}/asterisk/modules/cdr_csv.so
%{_libdir}/asterisk/modules/cdr_custom.so
%{_libdir}/asterisk/modules/cdr_manager.so
%{_libdir}/asterisk/modules/cdr_radius.so
%{_libdir}/asterisk/modules/cdr_syslog.so
%{_libdir}/asterisk/modules/cel_custom.so
%{_libdir}/asterisk/modules/cel_manager.so
%{_libdir}/asterisk/modules/cel_radius.so
%{_libdir}/asterisk/modules/chan_console.so
%{_libdir}/asterisk/modules/chan_iax2.so
%{_libdir}/asterisk/modules/chan_mgcp.so
%{_libdir}/asterisk/modules/chan_rtp.so
%{_libdir}/asterisk/modules/chan_oss.so
%{_libdir}/asterisk/modules/chan_phone.so
%{_libdir}/asterisk/modules/chan_skinny.so
%{_libdir}/asterisk/modules/chan_sip.so
%{_libdir}/asterisk/modules/chan_mobile.so
%{_libdir}/asterisk/modules/chan_unistim.so
%{_libdir}/asterisk/modules/codec_adpcm.so
%{_libdir}/asterisk/modules/codec_alaw.so
%{_libdir}/asterisk/modules/codec_a_mu.so
%{_libdir}/asterisk/modules/codec_g722.so
%{_libdir}/asterisk/modules/codec_g726.so
%{_libdir}/asterisk/modules/codec_gsm.so
%{_libdir}/asterisk/modules/codec_lpc10.so
%{_libdir}/asterisk/modules/codec_opus.so
%{_libdir}/asterisk/modules/codec_silk.so
%{_libdir}/asterisk/modules/codec_siren14.so
%{_libdir}/asterisk/modules/codec_siren7.so
%{_libdir}/asterisk/modules/codec_ulaw.so
%{_libdir}/asterisk/modules/codec_resample.so
%{_libdir}/asterisk/modules/format_g719.so
%{_libdir}/asterisk/modules/format_g723.so
%{_libdir}/asterisk/modules/format_g726.so
%{_libdir}/asterisk/modules/format_g729.so
%{_libdir}/asterisk/modules/format_gsm.so
%{_libdir}/asterisk/modules/format_h263.so
%{_libdir}/asterisk/modules/format_h264.so
%{_libdir}/asterisk/modules/format_ilbc.so
%{_libdir}/asterisk/modules/format_jpeg.so
%{_libdir}/asterisk/modules/format_ogg_opus.so
%{_libdir}/asterisk/modules/format_pcm.so
%{_libdir}/asterisk/modules/format_siren7.so
%{_libdir}/asterisk/modules/format_siren14.so
%{_libdir}/asterisk/modules/format_sln.so
%{_libdir}/asterisk/modules/format_vox.so
%{_libdir}/asterisk/modules/format_wav_gsm.so
%{_libdir}/asterisk/modules/format_wav.so
%{_libdir}/asterisk/modules/func_aes.so
%{_libdir}/asterisk/modules/func_base64.so
%{_libdir}/asterisk/modules/func_blacklist.so
%{_libdir}/asterisk/modules/func_callcompletion.so
%{_libdir}/asterisk/modules/func_callerid.so
%{_libdir}/asterisk/modules/func_cdr.so
%{_libdir}/asterisk/modules/func_channel.so
%{_libdir}/asterisk/modules/func_config.so
%{_libdir}/asterisk/modules/func_cut.so
%{_libdir}/asterisk/modules/func_db.so
%{_libdir}/asterisk/modules/func_devstate.so
%{_libdir}/asterisk/modules/func_dialgroup.so
%{_libdir}/asterisk/modules/func_dialplan.so
%{_libdir}/asterisk/modules/func_enum.so
%{_libdir}/asterisk/modules/func_env.so
%{_libdir}/asterisk/modules/func_extstate.so
%{_libdir}/asterisk/modules/func_frame_trace.so
%{_libdir}/asterisk/modules/func_global.so
%{_libdir}/asterisk/modules/func_groupcount.so
%{_libdir}/asterisk/modules/func_iconv.so
%{_libdir}/asterisk/modules/func_lock.so
%{_libdir}/asterisk/modules/func_logic.so
%{_libdir}/asterisk/modules/func_math.so
%{_libdir}/asterisk/modules/func_md5.so
%{_libdir}/asterisk/modules/func_module.so
%{_libdir}/asterisk/modules/func_pitchshift.so
%{_libdir}/asterisk/modules/func_periodic_hook.so
%{_libdir}/asterisk/modules/func_rand.so
%{_libdir}/asterisk/modules/func_realtime.so
%{_libdir}/asterisk/modules/func_sha1.so
%{_libdir}/asterisk/modules/func_shell.so
%{_libdir}/asterisk/modules/func_sprintf.so
%{_libdir}/asterisk/modules/func_srv.so
%{_libdir}/asterisk/modules/func_strings.so
%{_libdir}/asterisk/modules/func_sysinfo.so
%{_libdir}/asterisk/modules/func_timeout.so
%{_libdir}/asterisk/modules/func_uri.so
%{_libdir}/asterisk/modules/func_version.so
%{_libdir}/asterisk/modules/func_vmcount.so
%{_libdir}/asterisk/modules/func_volume.so
%{_libdir}/asterisk/modules/pbx_ael.so
%{_libdir}/asterisk/modules/pbx_config.so
%{_libdir}/asterisk/modules/pbx_dundi.so
%{_libdir}/asterisk/modules/pbx_loopback.so
%{_libdir}/asterisk/modules/pbx_lua.so
%{_libdir}/asterisk/modules/pbx_realtime.so
%{_libdir}/asterisk/modules/pbx_spool.so
%{_libdir}/asterisk/modules/res_adsi.so
%{_libdir}/asterisk/modules/res_ael_share.so
%{_libdir}/asterisk/modules/res_agi.so
%{_libdir}/asterisk/modules/res_calendar.so
%{_libdir}/asterisk/modules/res_calendar_caldav.so
%{_libdir}/asterisk/modules/res_calendar_ews.so
%{_libdir}/asterisk/modules/res_calendar_icalendar.so
%{_libdir}/asterisk/modules/res_chan_stats.so
%{_libdir}/asterisk/modules/res_clialiases.so
%{_libdir}/asterisk/modules/res_clioriginate.so
%{_libdir}/asterisk/modules/res_convert.so
%{_libdir}/asterisk/modules/res_config_sqlite.so
%{_libdir}/asterisk/modules/res_crypto.so
%{_libdir}/asterisk/modules/res_endpoint_stats.so
%{_libdir}/asterisk/modules/res_fax.so
%{_libdir}/asterisk/modules/res_format_attr_celt.so
%{_libdir}/asterisk/modules/res_format_attr_g729.so
%{_libdir}/asterisk/modules/res_format_attr_h263.so
%{_libdir}/asterisk/modules/res_format_attr_h264.so
%{_libdir}/asterisk/modules/res_format_attr_opus.so
%{_libdir}/asterisk/modules/res_format_attr_silk.so
%{_libdir}/asterisk/modules/res_format_attr_siren14.so
%{_libdir}/asterisk/modules/res_format_attr_siren7.so
%{_libdir}/asterisk/modules/res_format_attr_vp8.so
%{_libdir}/asterisk/modules/res_hep.so
%{_libdir}/asterisk/modules/res_hep_pjsip.so
%{_libdir}/asterisk/modules/res_hep_rtcp.so
%{_libdir}/asterisk/modules/res_http_post.so
%{_libdir}/asterisk/modules/res_limit.so
%{_libdir}/asterisk/modules/res_manager_devicestate.so
%{_libdir}/asterisk/modules/res_manager_presencestate.so
%{_libdir}/asterisk/modules/res_monitor.so
%{_libdir}/asterisk/modules/res_musiconhold.so
%{_libdir}/asterisk/modules/res_mutestream.so
%{_libdir}/asterisk/modules/res_phoneprov.so
%{_libdir}/asterisk/modules/res_realtime.so
%{_libdir}/asterisk/modules/res_rtp_asterisk.so
%{_libdir}/asterisk/modules/res_rtp_multicast.so
%{_libdir}/asterisk/modules/res_security_log.so
%{_libdir}/asterisk/modules/res_smdi.so
%{_libdir}/asterisk/modules/res_speech.so
%{_libdir}/asterisk/modules/res_stun_monitor.so
%{_libdir}/asterisk/modules/res_timing_pthread.so
%{_libdir}/asterisk/modules/res_timing_timerfd.so
%{_libdir}/asterisk/modules/func_speex.so
%{_libdir}/asterisk/modules/res_pktccops.so
%{_libdir}/asterisk/modules/res_config_ldap.so
%{_libdir}/asterisk/modules/res_fax_spandsp.so
%{_libdir}/asterisk/modules/codec_ilbc.so
%{_libdir}/asterisk/modules/func_jitterbuffer.so
%{_libdir}/asterisk/modules/func_presencestate.so
%{_libdir}/asterisk/modules/func_hangupcause.so
%{_libdir}/asterisk/modules/res_config_sqlite3.so
%{_libdir}/asterisk/modules/res_http_websocket.so
%{_libdir}/asterisk/modules/res_srtp.so
%{_libdir}/asterisk/modules/app_agent_pool.so
%{_libdir}/asterisk/modules/app_bridgewait.so
%{_libdir}/asterisk/modules/app_stasis.so
%{_libdir}/asterisk/modules/bridge_builtin_interval_features.so
%{_libdir}/asterisk/modules/bridge_holding.so
%{_libdir}/asterisk/modules/bridge_native_rtp.so
%{_libdir}/asterisk/modules/chan_bridge_media.so
%{_libdir}/asterisk/modules/chan_pjsip.so
%{_libdir}/asterisk/modules/func_pjsip_endpoint.so
%{_libdir}/asterisk/modules/res_ari.so
%{_libdir}/asterisk/modules/res_ari_applications.so
%{_libdir}/asterisk/modules/res_ari_asterisk.so
%{_libdir}/asterisk/modules/res_ari_bridges.so
%{_libdir}/asterisk/modules/res_ari_channels.so
%{_libdir}/asterisk/modules/res_ari_device_states.so
%{_libdir}/asterisk/modules/res_ari_endpoints.so
%{_libdir}/asterisk/modules/res_ari_events.so
%{_libdir}/asterisk/modules/res_ari_model.so
%{_libdir}/asterisk/modules/res_ari_playbacks.so
%{_libdir}/asterisk/modules/res_ari_recordings.so
%{_libdir}/asterisk/modules/res_ari_sounds.so
%{_libdir}/asterisk/modules/res_parking.so
%{_libdir}/asterisk/modules/res_pjsip.so
%{_libdir}/asterisk/modules/res_pjsip_acl.so
%{_libdir}/asterisk/modules/res_pjsip_authenticator_digest.so
%{_libdir}/asterisk/modules/res_pjsip_caller_id.so
%{_libdir}/asterisk/modules/res_pjsip_diversion.so
%{_libdir}/asterisk/modules/res_pjsip_dtmf_info.so
%{_libdir}/asterisk/modules/res_pjsip_dlg_options.so
%{_libdir}/asterisk/modules/res_pjsip_endpoint_identifier_anonymous.so
%{_libdir}/asterisk/modules/res_pjsip_endpoint_identifier_ip.so
%{_libdir}/asterisk/modules/res_pjsip_endpoint_identifier_user.so
%{_libdir}/asterisk/modules/res_pjsip_exten_state.so
%{_libdir}/asterisk/modules/res_pjsip_header_funcs.so
%{_libdir}/asterisk/modules/res_pjsip_logger.so
%{_libdir}/asterisk/modules/res_pjsip_messaging.so
%{_libdir}/asterisk/modules/res_pjsip_mwi.so
%{_libdir}/asterisk/modules/res_pjsip_nat.so
%{_libdir}/asterisk/modules/res_pjsip_notify.so
%{_libdir}/asterisk/modules/res_pjsip_one_touch_record_info.so
%{_libdir}/asterisk/modules/res_pjsip_outbound_authenticator_digest.so
%{_libdir}/asterisk/modules/res_pjsip_outbound_registration.so
%{_libdir}/asterisk/modules/res_pjsip_outbound_publish.so
%{_libdir}/asterisk/modules/res_pjsip_path.so
%{_libdir}/asterisk/modules/res_pjsip_pubsub.so
%{_libdir}/asterisk/modules/res_pjsip_publish_asterisk.so
%{_libdir}/asterisk/modules/res_pjsip_refer.so
%{_libdir}/asterisk/modules/res_pjsip_registrar.so
%{_libdir}/asterisk/modules/res_pjsip_rfc3326.so
%{_libdir}/asterisk/modules/res_pjsip_sdp_rtp.so
%{_libdir}/asterisk/modules/res_pjsip_session.so
%{_libdir}/asterisk/modules/res_pjsip_t38.so
%{_libdir}/asterisk/modules/res_pjsip_transport_websocket.so
%{_libdir}/asterisk/modules/res_sorcery_astdb.so
%{_libdir}/asterisk/modules/res_sorcery_memory_cache.so
%{_libdir}/asterisk/modules/res_sorcery_config.so
%{_libdir}/asterisk/modules/res_sorcery_memory.so
%{_libdir}/asterisk/modules/res_sorcery_realtime.so
%{_libdir}/asterisk/modules/res_stasis.so
%{_libdir}/asterisk/modules/res_stasis_answer.so
%{_libdir}/asterisk/modules/res_stasis_device_state.so
%{_libdir}/asterisk/modules/res_stasis_playback.so
%{_libdir}/asterisk/modules/res_stasis_recording.so
%{_libdir}/asterisk/modules/res_stasis_snoop.so
%{_libdir}/asterisk/modules/res_statsd.so
%{_libdir}/asterisk/modules/res_pjsip_mwi_body_generator.so
%{_libdir}/asterisk/modules/res_pjsip_pidf_body_generator.so
%{_libdir}/asterisk/modules/res_pjsip_pidf_eyebeam_body_supplement.so
%{_libdir}/asterisk/modules/res_pjsip_xpidf_body_generator.so
%{_libdir}/asterisk/modules/func_sorcery.so
%{_libdir}/asterisk/modules/res_pjsip_pidf_digium_body_supplement.so
%{_libdir}/asterisk/modules/res_pjsip_send_to_voicemail.so
%{_libdir}/asterisk/modules/func_talkdetect.so
%{_libdir}/asterisk/modules/res_pjsip_dialog_info_body_generator.so
%{_libdir}/asterisk/modules/res_pjsip_phoneprov_provider.so
%{_libdir}/asterisk/modules/func_pjsip_aor.so
%{_libdir}/asterisk/modules/func_pjsip_contact.so
%{_libdir}/asterisk/modules/res_pjsip_config_wizard.so
%{_libdir}/asterisk/modules/res_pjsip_sips_contact.so
%{_libdir}/asterisk/modules/func_holdintercept.so
%{_libdir}/asterisk/modules/res_odbc_transaction.so
%{_libdir}/asterisk/modules/res_pjproject.so
%{_libdir}/asterisk/modules/res_pjsip_history.so
%{_libdir}/asterisk/modules/res_pjsip_empty_info.so
%{_libdir}/asterisk/modules/*.xml
%{_libdir}/libasteriskpj.so
%{_libdir}/libasteriskpj.so.2

%{_sbindir}/asterisk
%{_sbindir}/rasterisk
%{_sbindir}/safe_asterisk
%{_sbindir}/astcanary
%{_sbindir}/astgenkey
%{_sbindir}/astdb2bdb
%{_sbindir}/astdb2sqlite3
%{_sbindir}/astversion

%{?_without_newt:%if 0}
%{!?_without_newt:%if 1}
#%{_sbindir}/astman
%endif
%{_sbindir}/autosupport
%{_libdir}/libasteriskssl.so.1
%{_libdir}/libasteriskssl.so

/var/lib/asterisk/scripts/*

%attr(0750,asterisk,asterisk) %dir %{logdir}/asterisk
%attr(0750,asterisk,asterisk) %dir %{logdir}/asterisk/cdr-csv
%attr(0750,asterisk,asterisk) %dir %{logdir}/asterisk/cdr-custom


#%config %{_sysconfdir}/logrotate.d/asterisk

%attr(0775,asterisk,asterisk) %dir /var/run/asterisk

%attr(0755,asterisk,asterisk) %dir /var/lib/asterisk
%attr(0755,asterisk,asterisk) %dir /var/lib/asterisk/agi-bin
%attr(0755,asterisk,asterisk) %dir /var/lib/asterisk/documentation
%attr(0644,asterisk,asterisk)      /var/lib/asterisk/documentation/*
%attr(0755,asterisk,asterisk) %dir /var/lib/asterisk/images
%attr(0644,asterisk,asterisk)      /var/lib/asterisk/images/*
%attr(0755,asterisk,asterisk) %dir /var/lib/asterisk/keys
%attr(0755,asterisk,asterisk) %dir /var/lib/asterisk/licenses
%attr(0755,asterisk,asterisk) %dir /var/lib/asterisk/phoneprov
%attr(0644,asterisk,asterisk)      /var/lib/asterisk/phoneprov/*
%attr(0755,asterisk,asterisk) %dir /var/lib/asterisk/static-http
%attr(0644,asterisk,asterisk)      /var/lib/asterisk/static-http/*

%attr(0755,asterisk,asterisk) %dir /var/lib/digium
%attr(0755,asterisk,asterisk) %dir /var/lib/digium/licenses

%attr(0755,asterisk,asterisk) %dir /var/lib/asterisk/rest-api
%attr(0644,asterisk,asterisk)      /var/lib/asterisk/rest-api/*

%attr(0775,asterisk,asterisk) %dir /var/spool/asterisk
%attr(0775,asterisk,asterisk) %dir /var/spool/asterisk/meetme
%attr(0775,asterisk,asterisk) %dir /var/spool/asterisk/system
%attr(0775,asterisk,asterisk) %dir /var/spool/asterisk/tmp
%attr(0775,asterisk,asterisk) %dir /var/spool/asterisk/voicemail

%files sounds
%attr(0755,asterisk,asterisk) %dir /var/lib/asterisk/sounds
%attr(0755,asterisk,asterisk) /var/lib/asterisk/sounds/*
%attr(0755,asterisk,asterisk) %dir /var/lib/asterisk/moh
%attr(0755,asterisk,asterisk) /var/lib/asterisk/moh/*


#
#  Alsa Subpackage
#
%{?_without_alsa:%if 0}
%{!?_without_alsa:%if 1}
%files alsa
%defattr(-, root, root)
%{_libdir}/asterisk/modules/chan_alsa.so
%endif

#
#  snmp Subpackage
#
%{?_without_snmp:%if 0}
%{!?_without_snmp:%if 1}
%files snmp
%defattr(-, root, root)
%{_libdir}/asterisk/modules/res_snmp.so
%endif

#
#  pgsql Subpackage
#
%{?_without_pgsql:%if 0}
%{!?_without_pgsql:%if 1}
%files pgsql
%defattr(-, root, root)
%{_libdir}/asterisk/modules/res_config_pgsql.so
%{_libdir}/asterisk/modules/cdr_pgsql.so
%{_libdir}/asterisk/modules/cel_pgsql.so
%endif

#
#  tds Subpackage
#
%{?_without_tds:%if 0}
%{!?_without_tds:%if 1}
%files tds
%defattr(-, root, root)
%{_libdir}/asterisk/modules/cdr_tds.so
%{_libdir}/asterisk/modules/cel_tds.so
%endif

#
#  mISDN Subpackage
#
%{?_without_misdn:%if 0}
%{!?_without_misdn:%if 1}
%files misdn
%defattr(-, root, root)
%{_libdir}/asterisk/modules/chan_misdn.so
%endif

#
#  dahdi Subpackage
#
%{?_without_dahdi:%if 0}
%{!?_without_dahdi:%if 1}
%files dahdi
%defattr(-, root, root)
%{_libdir}/asterisk/modules/app_dahdiras.so
%{_libdir}/asterisk/modules/app_flash.so
%{_libdir}/asterisk/modules/app_meetme.so
%{_libdir}/asterisk/modules/app_page.so
%{_libdir}/asterisk/modules/chan_dahdi.so
%{_libdir}/asterisk/modules/codec_dahdi.so
%{_libdir}/asterisk/modules/res_timing_dahdi.so
%{_datadir}/dahdi/span_config.d/40-asterisk
%endif

#
#  Configs Subpackage
#
%files configs
%defattr(-, asterisk, asterisk)
%attr(0664,asterisk,asterisk) %config(noreplace) %{_sysconfdir}/asterisk/*

#
#  cURL Subpackage
#
%{?_without_curl:%if 0}
%{!?_without_curl:%if 1}
%files curl
%defattr(-, root, root)
%{_libdir}/asterisk/modules/func_curl.so
%{_libdir}/asterisk/modules/res_config_curl.so
%{_libdir}/asterisk/modules/res_curl.so
%endif

#
#  Development Subpackage
#
%files devel
%defattr(-, root, root)
%{_includedir}/asterisk.h
%{_includedir}/asterisk/alertpipe.h
%{_includedir}/asterisk/spinlock.h
%{_includedir}/asterisk/_private.h
%{_includedir}/asterisk/abstract_jb.h
%{_includedir}/asterisk/acl.h
%{_includedir}/asterisk/adsi.h
%{_includedir}/asterisk/ael_structs.h
%{_includedir}/asterisk/agi.h
%{_includedir}/asterisk/alaw.h
%{_includedir}/asterisk/aoc.h
%{_includedir}/asterisk/autochan.h
%{_includedir}/asterisk/autoconfig.h
%{_includedir}/asterisk/app.h
%{_includedir}/asterisk/astdb.h
%{_includedir}/asterisk/ast_expr.h
%{_includedir}/asterisk/ast_version.h
%{_includedir}/asterisk/astmm.h
%{_includedir}/asterisk/astobj.h
%{_includedir}/asterisk/astobj2.h
%{_includedir}/asterisk/astosp.h
%{_includedir}/asterisk/audiohook.h
%{_includedir}/asterisk/build.h
%{_includedir}/asterisk/buildinfo.h
%{_includedir}/asterisk/buildopts.h
%{_includedir}/asterisk/calendar.h
%{_includedir}/asterisk/callerid.h
%{_includedir}/asterisk/causes.h
%{_includedir}/asterisk/ccss.h
%{_includedir}/asterisk/cdr.h
%{_includedir}/asterisk/cel.h
%{_includedir}/asterisk/channel.h
%{_includedir}/asterisk/channelstate.h
%{_includedir}/asterisk/chanvars.h
%{_includedir}/asterisk/cli.h
%{_includedir}/asterisk/compat.h
%{_includedir}/asterisk/compiler.h
%{_includedir}/asterisk/config.h
%{_includedir}/asterisk/crypto.h
%{_includedir}/asterisk/data.h
%{_includedir}/asterisk/datastore.h
%{_includedir}/asterisk/devicestate.h
%{_includedir}/asterisk/dial.h
%{_includedir}/asterisk/dlinkedlists.h
%{_includedir}/asterisk/dns.h
%{_includedir}/asterisk/dnsmgr.h
%{_includedir}/asterisk/doxyref.h
%{_includedir}/asterisk/dsp.h
%{_includedir}/asterisk/dundi.h
%{_includedir}/asterisk/endian.h
%{_includedir}/asterisk/enum.h
%{_includedir}/asterisk/event.h
%{_includedir}/asterisk/event_defs.h
%{_includedir}/asterisk/extconf.h
%{_includedir}/asterisk/features.h
%{_includedir}/asterisk/file.h
%{_includedir}/asterisk/frame.h
%{_includedir}/asterisk/framehook.h
%{_includedir}/asterisk/fskmodem.h
%{_includedir}/asterisk/fskmodem_float.h
%{_includedir}/asterisk/fskmodem_int.h
%{_includedir}/asterisk/hashtab.h
%{_includedir}/asterisk/heap.h
%{_includedir}/asterisk/http.h
%{_includedir}/asterisk/image.h
%{_includedir}/asterisk/indications.h
%{_includedir}/asterisk/inline_api.h
%{_includedir}/asterisk/io.h
%{_includedir}/asterisk/global_datastores.h
%{_includedir}/asterisk/linkedlists.h
%{_includedir}/asterisk/localtime.h
%{_includedir}/asterisk/lock.h
%{_includedir}/asterisk/logger.h
%{_includedir}/asterisk/manager.h
%{_includedir}/asterisk/md5.h
%{_includedir}/asterisk/mod_format.h
%{_includedir}/asterisk/module.h
%{_includedir}/asterisk/monitor.h
%{_includedir}/asterisk/musiconhold.h
%{_includedir}/asterisk/netsock.h
%{_includedir}/asterisk/netsock2.h
%{_includedir}/asterisk/network.h
%{_includedir}/asterisk/optional_api.h
%{_includedir}/asterisk/options.h
%{_includedir}/asterisk/paths.h
%{_includedir}/asterisk/pbx.h
%{_includedir}/asterisk/pktccops.h
%{_includedir}/asterisk/plc.h
%{_includedir}/asterisk/poll-compat.h
%{_includedir}/asterisk/privacy.h
%{_includedir}/asterisk/pval.h
%{_includedir}/asterisk/res_odbc.h
%{_includedir}/asterisk/res_fax.h
%{_includedir}/asterisk/res_srtp.h
%{_includedir}/asterisk/rtp_engine.h
%{_includedir}/asterisk/say.h
%{_includedir}/asterisk/sched.h
%{_includedir}/asterisk/security_events.h
%{_includedir}/asterisk/security_events_defs.h
%{_includedir}/asterisk/select.h
%{_includedir}/asterisk/sha1.h
%{_includedir}/asterisk/slin.h
%{_includedir}/asterisk/slinfactory.h
%{_includedir}/asterisk/smdi.h
%{_includedir}/asterisk/speech.h
%{_includedir}/asterisk/srv.h
%{_includedir}/asterisk/stringfields.h
%{_includedir}/asterisk/strings.h
%{_includedir}/asterisk/stun.h
%{_includedir}/asterisk/syslog.h
%{_includedir}/asterisk/taskprocessor.h
%{_includedir}/asterisk/tcptls.h
%{_includedir}/asterisk/tdd.h
%{_includedir}/asterisk/term.h
%{_includedir}/asterisk/test.h
%{_includedir}/asterisk/threadstorage.h
%{_includedir}/asterisk/time.h
%{_includedir}/asterisk/timing.h
%{_includedir}/asterisk/transcap.h
%{_includedir}/asterisk/translate.h
%{_includedir}/asterisk/udptl.h
%{_includedir}/asterisk/ulaw.h
%{_includedir}/asterisk/unaligned.h
%{_includedir}/asterisk/utils.h
%{_includedir}/asterisk/vector.h
%{_includedir}/asterisk/version.h
%{_includedir}/asterisk/xml.h
%{_includedir}/asterisk/xmldoc.h
%{_includedir}/asterisk/doxygen/architecture.h
%{_includedir}/asterisk/doxygen/licensing.h
%{_includedir}/asterisk/celt.h
%{_includedir}/asterisk/format.h
%{_includedir}/asterisk/format_cap.h
%{_includedir}/asterisk/message.h
%{_includedir}/asterisk/silk.h
%{_includedir}/asterisk/presencestate.h
%{_includedir}/asterisk/channel_internal.h
%{_includedir}/asterisk//config_options.h
%{_includedir}/asterisk/http_websocket.h
%{_includedir}/asterisk/sip_api.h
%{_includedir}/asterisk/xmpp.h
%{_includedir}/asterisk/ari.h
%{_includedir}/asterisk/backtrace.h
%{_includedir}/asterisk/bridge.h
%{_includedir}/asterisk/bridge_after.h
%{_includedir}/asterisk/bridge_basic.h
%{_includedir}/asterisk/bridge_channel.h
%{_includedir}/asterisk/bridge_channel_internal.h
%{_includedir}/asterisk/bridge_features.h
%{_includedir}/asterisk/bridge_internal.h
%{_includedir}/asterisk/bridge_roles.h
%{_includedir}/asterisk/bridge_technology.h
%{_includedir}/asterisk/bucket.h
%{_includedir}/asterisk/core_local.h
%{_includedir}/asterisk/core_unreal.h
%{_includedir}/asterisk/endpoints.h
%{_includedir}/asterisk/features_config.h
%{_includedir}/asterisk/json.h
%{_includedir}/asterisk/media_index.h
%{_includedir}/asterisk/mixmonitor.h
%{_includedir}/asterisk/opus.h
%{_includedir}/asterisk/parking.h
%{_includedir}/asterisk/pickup.h
%{_includedir}/asterisk/res_pjsip.h
%{_includedir}/asterisk/res_pjsip_cli.h
#%{_includedir}/asterisk/res_pjsip_exten_state.h
%{_includedir}/asterisk/res_pjsip_pubsub.h
%{_includedir}/asterisk/res_pjsip_session.h
%{_includedir}/asterisk/sdp_srtp.h
%{_includedir}/asterisk/sem.h
%{_includedir}/asterisk/sorcery.h
%{_includedir}/asterisk/sounds_index.h
%{_includedir}/asterisk/stasis.h
%{_includedir}/asterisk/stasis_app.h
%{_includedir}/asterisk/stasis_app_device_state.h
%{_includedir}/asterisk/stasis_app_impl.h
%{_includedir}/asterisk/stasis_app_playback.h
%{_includedir}/asterisk/stasis_app_recording.h
%{_includedir}/asterisk/stasis_app_snoop.h
%{_includedir}/asterisk/stasis_bridges.h
%{_includedir}/asterisk/stasis_cache_pattern.h
%{_includedir}/asterisk/stasis_channels.h
%{_includedir}/asterisk/stasis_endpoints.h
%{_includedir}/asterisk/stasis_internal.h
%{_includedir}/asterisk/stasis_message_router.h
%{_includedir}/asterisk/stasis_system.h
%{_includedir}/asterisk/stasis_test.h
%{_includedir}/asterisk/statsd.h
%{_includedir}/asterisk/threadpool.h
%{_includedir}/asterisk/uuid.h
%{_includedir}/asterisk/res_mwi_external.h
%{_includedir}/asterisk/stasis_app_mailbox.h
%{_includedir}/asterisk/res_pjsip_body_generator_types.h
%{_includedir}/asterisk/res_pjsip_presence_xml.h
%{_includedir}/asterisk/res_hep.h
%{_includedir}/asterisk/beep.h
%{_includedir}/asterisk/codec.h
%{_includedir}/asterisk/format_cache.h
%{_includedir}/asterisk/format_compatibility.h
%{_includedir}/asterisk/res_pjsip_outbound_publish.h
%{_includedir}/asterisk/smoother.h
%{_includedir}/asterisk/uri.h
%{_includedir}/asterisk/phoneprov.h
%{_includedir}/asterisk/max_forwards.h
%{_includedir}/asterisk/res_odbc_transaction.h
%{_includedir}/asterisk/res_pjproject.h
%{_includedir}/asterisk/named_locks.h
%{_includedir}/asterisk/multicast_rtp.h

#
#  Documentation Subpackage
#
%files doc
%defattr(-, root, root)

#
#  Manual Pages
#
%{_mandir}

#
#  Ogg-Vorbis Subpackage
#
%{?_without_ogg:%if 0}
%{!?_without_ogg:%if 1}
%files ogg
%defattr(-, root, root)
%{_libdir}/asterisk/modules/format_ogg_vorbis.so
%endif

#
#  Speex Subpackage
#
%{?_without_speex:%if 0}
%{!?_without_speex:%if 1}
%files speex
%defattr(-, root, root)
%{_libdir}/asterisk/modules/codec_speex.so
%endif

#
#  resample Subpackage
#
%{?_without_resample:%if 0}
%{!?_without_resample:%if 1}
%files resample
%defattr(-, root, root)
%{_libdir}/asterisk/modules/codec_resample.so
%endif

#
#  unixODBC Subpackage
#
%{?_without_odbc:%if 0}
%{!?_without_odbc:%if 1}
%files odbc
%defattr(-, root, root)
%{_libdir}/asterisk/modules/cdr_adaptive_odbc.so
%{_libdir}/asterisk/modules/cdr_odbc.so
%{_libdir}/asterisk/modules/cel_odbc.so
%{_libdir}/asterisk/modules/func_odbc.so
%{_libdir}/asterisk/modules/res_config_odbc.so
%{_libdir}/asterisk/modules/res_odbc.so
%endif

#
#  sqlite3 Subpackage
#
%{?_without_sqlite3:%if 0}
%{!?_without_sqlite3:%if 1}
%files sqlite3
%defattr(-, root, root)
%{_libdir}/asterisk/modules/cdr_sqlite3_custom.so
%{_libdir}/asterisk/modules/cel_sqlite3_custom.so
%endif

#
#  voicemail file storage Subpackage
#
%files voicemail
%defattr(-, root, root)
%{_libdir}/asterisk/modules/app_voicemail.so

#
#  voicemail ODBC storage Subpackage
#
%{?_without_voicemail_odbcstorage:%if 0}
%{!?_without_voicemail_odbcstorage:%if 1}
%files voicemail-odbcstorage
%defattr(-, root, root)
%{_libdir}/asterisk/modules/app_voicemail_odbcstorage.so
%endif

#
#  voicemail IMAP storage Subpackage
#
%{?_without_voicemail_imapstorage:%if 0}
%{!?_without_voicemail_imapstorage:%if 1}
%files voicemail-imapstorage
%defattr(-, root, root)
%{_libdir}/asterisk/modules/app_voicemail_imapstorage.so
%endif

%files addons
%defattr(-, root, root)

# %files addons-core
#%defattr(-, root, root)
#%{_libdir}/asterisk/modules/format_mp3.so

%{?_without_mysql:%if 0}
%{!?_without_mysql:%if 1}
%files addons-mysql
%{_libdir}/asterisk/modules/app_mysql.so
%{_libdir}/asterisk/modules/cdr_mysql.so
%{_libdir}/asterisk/modules/res_config_mysql.so
%endif

%{?_without_bluetooth:%if 0}
%{!?_without_bluetooth:%if 1}
%files addons-bluetooth
%{_libdir}/asterisk/modules/chan_mobile.so
%endif

%{?_without_ooh323:%if 0}
%{!?_without_ooh323:%if 1}
%files addons-ooh323
%{_libdir}/asterisk/modules/chan_ooh323.so
%endif

%changelog
* Thu Jul 19 2018 Emmanuel BUU <emmanuel.buu@ives.fr> - 13.22.0
- used this .spec file for IVeS packaging.

* Thu Jan 25 2018 Giacomo Sanchietti <giacomo.sanchietti@nethesis.it> - 13.17.2-3-1
- PBX: Asterisk logs aren't rotated - Bug NethServer/dev#5411

* Wed Nov 15 2017 Stefano Fancello <stefano.fancello@nethesis.it> 13.17.2-2
- Change Asterisk user home from /var/lib/asterisk to /home/asterisk

