Summary: NSS update DB tool
Name: nss_updatedb
Version: 1
Release: 1
Source0: ftp://ftp.padl.com/pub/%{name}-%{version}.tar.gz
URL: http://www.padl.com/
Copyright: GPL
Group: System Environment/Base
BuildRoot: %{_tmppath}/%{name}-root
Requires: nss_db
Obsoletes: nss_updatedb

%description
The nss_updatedb tool synchronizes nameservice information
with a local Berkeley DB cache.

%prep
%setup -q -a 0

%build
./configure
make

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/{sbin}

install -m 700 nss_updatedb $RPM_BUILD_ROOT/sbin

chmod 755 $RPM_BUILD_ROOT/sbin/nss_updatedb

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%attr(0755,root,root) /sbin/nss_updatedb
%doc AUTHORS NEWS COPYING README ChangeLog

%changelog

