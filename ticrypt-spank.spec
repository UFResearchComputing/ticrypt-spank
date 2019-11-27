Name:       ticrypt-spank	
Version:    1.2
Release:    3
Summary:    Ticrypt spank plugin for Slurm

License:    GPLv3	
URL:        https://github.com/UFResearchComputing/ticrypt-spank
Source0:    https://github.com/UFResearchComputing/ticrypt-spank/archive/v%{version}.tar.gz

BuildRequires:	gcc slurm-devel libconfig-devel git
Requires:	slurm libconfig

%description
Provides Slurm Spank and submit plugins to allow node reconfiguration for scheduling Ticrypt VM Hosts.

%{expand:%%define default_slurm_version slurm-19-05-4-1}
%{!?slurm_version: %define slurm_version %{default_slurm_version}}
%{expand:%%define default_lib /opt/slurm/lib64/slurm}
%{!?lib: %define lib %{default_lib}}
%{expand:%%define default_inc /opt/slurm/include}
%{!?inc: %define inc %{default_inc}}
%{expand:%%define default_man /usr/local/share/man/man8}
%{!?man: %define man %{default_man}}

%prep
%setup -q 

%install
mkdir -p %{buildroot}/etc
mkdir -p %{buildroot}/%{man}
mkdir -p %{buildroot}/%{lib}
cd %_builddir/ticrypt-spank-%{version}
make all LIB=%{lib} SLURM_VERSION=%{slurm_version} INC=%{inc}
cp ticrypt.so %{buildroot}/%{lib}/
cp config/ticrypt-spank.conf %{buildroot}/etc/
cp doc/ticrypt-spank.8.gz %{buildroot}/%{man}
cp job_submit_ticrypt.so %{buildroot}/%{lib}/

%clean
rm -rf %_builddir/ticrypt-spank-%{version}
rm -rf %{buildroot}

%files
%attr(0755,root,root) %{lib}/ticrypt.so
%attr(0755,root,root) %{lib}/job_submit_ticrypt.so
%attr(0640,root,root) /etc/ticrypt-spank.conf
%attr(0644,root,root) %{man}/ticrypt-spank.8.gz
%doc

%changelog
* Wed Nov 27 2019 William Howell <whowell@rc.ufl.edu> - 1.2-3
- Fix up build docs
* Wed Nov 27 2019 William Howell <whowell@rc.ufl.edu> - 1.2
- Add job submit plugin
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

