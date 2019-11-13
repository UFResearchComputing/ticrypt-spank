Name:	ticrypt-spank	
Version:	1
Release:	1
Summary:	Ticrypt spank plugin for Slurm

License:    GPLv3	
URL:		https://github.com/UFResearchComputing/ticrypt-spank
Source0:	https://github.com/UFResearchComputing/ticrypt-spank/archive/ticrypt-spank-%{version}-%{release}.tar.gz

BuildRequires:	gcc slurm-devel libconfig-devel
Requires:	slurm libconfig

%description
Provides a Slurm Spank plugin to allow node reconfiguration for scheduling Ticrypt VM Hosts.

%prep
%setup -q 

%install
mkdir -p %{buildroot}/opt/slurm/lib64/slurm
mkdir -p %{buildroot}/etc
mkdir -p %{buildroot}/usr/local/share/man/man8
cd %_builddir/ticrypt-spank-%{version}
make install DESTDIR=/opt/slurm
cp ticrypt.so %{buildroot}/opt/slurm/lib64/slurm/
cp config/ticrypt-spank.conf %{buildroot}/etc/
cp doc/ticrypt-spank.8.gz %{buildroot}/usr/local/share/man/man8/

%clean
rm -rf %_builddir/ticrypt-spank-%{version}
rm -rf %{buildroot}

%files
%attr(0755,root,root) /opt/slurm/lib64/slurm/ticrypt.so
%attr(0640,root,root) /etc/ticrypt-spank.conf
%attr(0644,root,root) /usr/local/share/man/man8/ticrypt-spank.8.gz
%doc

%changelog
* Wed Nov 13 2019 William Howell <whowell@rc.ufl.edu> - 1.1
- Version 1.1 release
* Sun Nov 10 2019 William Howell <whowell@rc.ufl.edu> - 1.0
- Version 1.0 release
* Sat Nov 9 2019 William Howell <whowell@rc.ufl.edu> - 0.3
- Add resume state on successful cleanup
* Fri Oct 18 2019 William Howell <whowell@rc.ufl.edu> - 0.2
- Rework of structure
* Mon Oct 14 2019 William Howell <whowell@rc.ufl.edu> - 0.1
- Initial packaging

