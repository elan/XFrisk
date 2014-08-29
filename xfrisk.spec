# Initial spec file created by autospec ver. 0.3
Summary: XFrisk, a multi-user network version of the classic "Risk"
# The Summary: line should be expanded to about here -----^
Name: XFrisk
%define version 1.2
Version: %{version}
Release: 1
Group: Amusements/Games
Copyright: GPL
Source: ./XFrisk-1.2.tar.gz
#BuildRoot: /tmp/XFrisk-root
# Following are optional fields
URL: http://www.iki.fi/morphy/xfrisk/XFrisk-1.2-1.rpm
#Distribution: Red Hat Contrib-Net
#Patch: XFrisk.patch
#Prefix: /usr/local
#BuildArchitectures: noarch
#Requires: 
#Obsoletes: 

%description
XFrisk version 1.2

%prep
%setup -n XFrisk
#%patch

%build
make

%install
make install

%files
/usr/local/lib/xfrisk/World.risk
/usr/local/lib/xfrisk/Countries.risk
/usr/local/lib/xfrisk/Help.risk
/usr/local/bin/risk
/usr/local/bin/xfrisk
/usr/local/bin/friskserver
/usr/local/bin/aiDummy
/usr/local/bin/aiConway
/usr/local/bin/aiColson
%doc BUGS
%doc COPYING
%doc ChangeLog
%doc FAQ
%doc README.NEW
%doc TODO
