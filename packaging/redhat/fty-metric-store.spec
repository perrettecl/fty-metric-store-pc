#
#    fty-metric-store - Persistance for metrics
#
#    Copyright (C) 2014 - 2018 Eaton
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License along
#    with this program; if not, write to the Free Software Foundation, Inc.,
#    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#

# To build with draft APIs, use "--with drafts" in rpmbuild for local builds or add
#   Macros:
#   %_with_drafts 1
# at the BOTTOM of the OBS prjconf
%bcond_with drafts
%if %{with drafts}
%define DRAFTS yes
%else
%define DRAFTS no
%endif
%define SYSTEMD_UNIT_DIR %(pkg-config --variable=systemdsystemunitdir systemd)
Name:           fty-metric-store
Version:        1.0.0
Release:        1
Summary:        persistance for metrics
License:        GPL-2.0+
URL:            https://42ity.org
Source0:        %{name}-%{version}.tar.gz
Group:          System/Libraries
# Note: ghostscript is required by graphviz which is required by
#       asciidoc. On Fedora 24 the ghostscript dependencies cannot
#       be resolved automatically. Thus add working dependency here!
BuildRequires:  ghostscript
BuildRequires:  asciidoc
BuildRequires:  automake
BuildRequires:  autoconf
BuildRequires:  libtool
BuildRequires:  pkgconfig
BuildRequires:  systemd-devel
BuildRequires:  systemd
%{?systemd_requires}
BuildRequires:  xmlto
# Note that with current implementation of zproject use-cxx-gcc-4-9 option,
# this effectively hardcodes the use of specifically 4.9, not allowing for
# "4.9 or newer".
BuildRequires:  devtoolset-3-gcc devtoolset-3-gcc-c++
BuildRequires:  gcc-c++ >= 4.9.0
BuildRequires:  libsodium-devel
BuildRequires:  zeromq-devel
BuildRequires:  czmq-devel >= 3.0.2
BuildRequires:  malamute-devel >= 1.0.0
BuildRequires:  fty-common-logging-devel
BuildRequires:  fty-proto-devel >= 1.0.0
BuildRequires:  cxxtools-devel
BuildRequires:  tntdb-devel
BuildRequires:  fty_shm-devel >= 1.0.0
BuildRoot:      %{_tmppath}/%{name}-%{version}-build

%description
fty-metric-store persistance for metrics.

%package -n libfty_metric_store0
Group:          System/Libraries
Summary:        persistance for metrics shared library

%description -n libfty_metric_store0
This package contains shared library for fty-metric-store: persistance for metrics

%post -n libfty_metric_store0 -p /sbin/ldconfig
%postun -n libfty_metric_store0 -p /sbin/ldconfig

%files -n libfty_metric_store0
%defattr(-,root,root)
%doc COPYING
%{_libdir}/libfty_metric_store.so.*

%package devel
Summary:        persistance for metrics
Group:          System/Libraries
Requires:       libfty_metric_store0 = %{version}
Requires:       libsodium-devel
Requires:       zeromq-devel
Requires:       czmq-devel >= 3.0.2
Requires:       malamute-devel >= 1.0.0
Requires:       fty-common-logging-devel
Requires:       fty-proto-devel >= 1.0.0
Requires:       cxxtools-devel
Requires:       tntdb-devel
Requires:       fty_shm-devel >= 1.0.0

%description devel
persistance for metrics development tools
This package contains development files for fty-metric-store: persistance for metrics

%files devel
%defattr(-,root,root)
%{_includedir}/*
%{_libdir}/libfty_metric_store.so
%{_libdir}/pkgconfig/libfty_metric_store.pc
%{_mandir}/man3/*
%{_mandir}/man7/*

%prep

%setup -q

%build
sh autogen.sh
%{configure} --enable-drafts=%{DRAFTS} --with-systemd-units
make %{_smp_mflags}

%install
make install DESTDIR=%{buildroot} %{?_smp_mflags}

# remove static libraries
find %{buildroot} -name '*.a' | xargs rm -f
find %{buildroot} -name '*.la' | xargs rm -f

%files
%defattr(-,root,root)
%doc README.md
%doc COPYING
%{_bindir}/fty-metric-store
%{_mandir}/man1/fty-metric-store*
%{_bindir}/fty-metric-store-cleaner
%config(noreplace) %{_sysconfdir}/fty-metric-store/fty-metric-store.cfg
%{SYSTEMD_UNIT_DIR}/fty-metric-store.service
%{SYSTEMD_UNIT_DIR}/fty-metric-store-cleaner.service
%{SYSTEMD_UNIT_DIR}/fty-metric-store-cleaner.timer
%dir %{_sysconfdir}/fty-metric-store
%if 0%{?suse_version} > 1315
%post
%systemd_post fty-metric-store.service fty-metric-store-cleaner.service fty-metric-store-cleaner.timer
%preun
%systemd_preun fty-metric-store.service fty-metric-store-cleaner.service fty-metric-store-cleaner.timer
%postun
%systemd_postun_with_restart fty-metric-store.service fty-metric-store-cleaner.service fty-metric-store-cleaner.timer
%endif

%changelog
