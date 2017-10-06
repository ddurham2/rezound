%define name	rezound
%define version	0.13.0beta
%define release 1

Summary:	ReZound is a sound file editor and recorder
Name:		%{name}
Version: 	%{version}
Release: 	%{release}
License: 	GPL
Group: 		Sound
URL: 		http://rezound.sourceforge.net/
Source: 	http://prdownloads.sourceforge.net/%{name}/%{name}-%{version}.tar.gz
BuildRoot: 	%{_tmppath}/%{name}-%{version}-%{release}-buildroot


%description
ReZound aims to be a stable, open source, and graphical audio file editor 
primarily for but not limited to the Linux operating system.  This rpm is
based on the statically linked binary so as to eliminate nearly all
dependancies.


%prep
rm -rf $RPM_BUILD_ROOT
%setup -n %{name}-%{version} 

%build
%configure --enable-standalone --enable-internal-sample-type=float
make


%install
%makeinstall PREFIX=$RPM_BUILD_ROOT/usr install-kde-parts

%clean
#rm -rf %buildroot/
rm -rf $RPM_BUILD_ROOT

%post

%postun

%files
%defattr(-,root,root,0755)
%doc /usr/doc/%name/AUTHORS
%doc /usr/doc/%name/COPYING
%doc /usr/doc/%name/NEWS
%doc /usr/doc/%name/README
%doc /usr/doc/%name/FrontendFoxFeatures.txt
%doc /usr/doc/%name/Features.txt
%_datadir/locale/ru/LC_MESSAGES/%name.mo
%_datadir/locale/de/LC_MESSAGES/%name.mo
%_datadir/locale/es/LC_MESSAGES/%name.mo
%_datadir/%name/* 
%_bindir/%name
# kde stuff
%_datadir/mimelnk/audio/x-rez.desktop
%_datadir/mimelnk/audio/x-rez.kdelnk
%_datadir/applnk/Multimedia/rezound.desktop


%changelog
* Mon Oct 21 2004 Davy Durham <ddurham@users.sourceforge.com> 0.11.0beta-1
- twelvth rpm release

* Mon Jul 19 2004 Davy Durham <ddurham@users.sourceforge.com> 0.10.0beta-1
- eleventh rpm release

* Mon Nov 7 2003 Davy Durham <ddurham@users.sourceforge.com> 0.9.0beta-1
- tenth rpm release

* Mon Jul 14 2003 Davy Durham <ddurham@users.sourceforge.com> 0.8.3beta-1
- ninth rpm release

* Tue Jul 8 2003 Davy Durham <ddurham@users.sourceforge.com> 0.8.2beta-1
- eighth rpm release

* Fri Jun 26 2003 Davy Durham <ddurham@users.sourceforge.com> 0.8.1beta-1
- seventh rpm release

* Wed Jun 24 2003 Davy Durham <ddurham@users.sourceforge.com> 0.8.0beta-1
- sixth rpm release

* Wed Feb 24 2003 Davy Durham <ddurham@users.sourceforge.com> 0.7.0beta-1
- fifth rpm release

* Wed Jan 27 2003 Davy Durham <ddurham@users.sourceforge.com> 0.6.0beta-1
- forth rpm release

* Wed Oct 27 2002 Davy Durham <ddurham@users.sourceforge.com> 0.5.1beta-1
- third rpm release

* Wed Oct 23 2002 Davy Durham <ddurham@users.sourceforge.com> 0.5.0beta-1
- second rpm release

* Thu Sep 6 2002 Davy Durham <ddurham@users.sourceforge.com> 0.4.0beta-1
- first rpm release

