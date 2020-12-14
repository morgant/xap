Summary:	X Application Panel and X Window Finder
Name:		xap
Version:	0.9.7
Release:	1
Copyright:	GNU GPL
Group:		User Interface/X
Url:		http://home.pages.de/~rasca/xap/
Source:		http://home.pages.de/~rasca/%{name}-%{version}.tar.gz
Packager:	Rasca <thron@gmx.de>
Buildroot:	%{__tmppath}/%{name}-buildroot
Prefix:		%{__prefix}

%description
xap - gtk+ based application panel for X11.
xwf - gtk+ based filemanager for X Window.
xpg - gtk+ based frontend for gnupg
xcp - gtk+ based frontend for cp/mv
xfi - gtk+ based frontend for find

%prep
%setup -q -n xap

%build
%configure --with-urlfetch=curl
make

%install
rm -rf $RPM_BUILD_ROOT
make install \
	prefix=$RPM_BUILD_ROOT%{_prefix} \
	icondir=$RPM_BUILD_ROOT%{_prefix}/share/icons

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,bin,bin)
%doc COPYING ChangeLog INSTALL README TODO
%doc docs/Mlvwmrc-Xwf
%doc index.html docs/*.png docs/*.css docs/*.gif
%doc %{_mandir}/*/*
%{_prefix}/bin/*
%{_prefix}/share/icons/*

%changelog

