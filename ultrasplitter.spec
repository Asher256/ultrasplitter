Summary: Diviser les fichiers en plusieurs morceaux
Name: ultrasplitter
Version: 3.2
Release: 1
License: GPL
Packager: Asher256 <asher256@gmail.com>
Group: Applications/File
Source0: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-buildroot
URL: http://jargon.asher256.com/

%description
UltraSplitter est un logiciel fait pour diviser les fichiers en plusieurs 
morceaux pour pouvoir les mettres sur des supports comme les disquette 
1.44 mo, des sylos memoire de 64 mo, etc. Tres utile quand on deplace un 
fichier depassant la limite de notre support (par exemple l'image d'un CD 
de 700 MO devant être deplace avec un stylo memoire USB ne contenant que 
64 MO). 

%prep
%setup -q

%build
./fixlinux.sh
make

%install
rm -fr $RPM_BUILD_ROOT
make install PREFIX=$RPM_BUILD_ROOT/usr

%clean
rm -fr $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{_bindir}/ultrasplitter
%{_datadir}/doc/ultrasplitter/LISEZMOI.txt
%{_datadir}/doc/ultrasplitter/COPYING.txt
%{_datadir}/doc/ultrasplitter/COPYING-FR.txt
%{_datadir}/doc/ultrasplitter/html/*
%{_datadir}/applications/ultrasplitter.desktop
%{_datadir}/pixmaps/ultrasplitter.png

%changelog
* Sun Nov 6 2005 Asher256 <asher256@gmail.com> 3.1 
- First version of the .spec

