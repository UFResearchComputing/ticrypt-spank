# ticrypt-spank

[Slurm](https://www.schedmd.com/) SPANK and job submit plugins to allow calling cluster specific modules for node reconfiguration to [TiCrypt](https://terainsights.com/) VM hosts. The plugin adds the --ticrypt Slurm option for requesting a TiCrypt node. The plugin utilizes a configuration file to enforce limits or requirements on accounts, features, etc ... which are allowed to call the plugin.  The configuration file is shared and required for both the spank and job submit plugins.

## Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes. See deployment for notes on how to deploy the project on a live system.

### Prerequisites

This SPANK plugin requires [libconfig](http://hyperrealm.github.io/libconfig/) which is broadly availble under common Linux distributions. Utilizing the plugin requires a functional [Slurm](https://www.schedmd.com/) installation with [plugstack](https://slurm.schedmd.com/spank.html) configuration. Building the packages or development requires slurm-devel as well as a checked out copy of the [slurm source](https://github.com/SchedMD/slurm) of correct version for your system in order to build the job submit plugin. 

### Installation

#### Manual Installation

* Clone the repository

* Copy the example configuration to system

```
cp config/ticrypt-spank.conf
``` 

* Clone the slurm submodule into the checked out repository and run the configure for Slurm. Note that the included Makefile will checkout the desired branch as shown later by passing SLURM_VERSION to make, so the checkout and configure below are not technically required but shown for reference. 

```
cd ticrypt-spank/
git submodule add --force https://github.com/SchedMD/slurm.git slurm
cd slurm
checkout <desired slurm branch>
./configure
cd ..
```

* Build the plugin optionally specifying DESTDIR if required for your environment

```
# example with Slurm 19-05-4-1 installed in /opt/slurm

cd ticrypt-spank/
make install LIB=/opt/slurm/lib64/slurm SLURM_VERSION=slurm-19-05-4-1 INC=/opt/slurm/include
```

#### RPM Build and Install
* Setup a build environment per system requirements such as [CentOS](https://wiki.centos.org/HowTos/SetupRpmBuildEnvironment)

* In the steps below, <major> should be substitued by the desired major release, and <minor> substituted by the desired minor release

* Download desired release to SOURCES
```
cd $HOME/rpmbuild/SOURCES
wget https://github.com/UFResearchComputing/ticrypt-spank/archive/v<major>.<minor>.tar.gz
```

* Copy the specfile from the source to SPECS
```
cd $HOME/rpmbuild/SOURCES
tar -xf v<major>.<minor>.tar.gz 
cp ticrypt-spank-<major>.<minor>/ticrypt-spank.spec $HOME/rpmbuild/SPECS
```

* Modify specifle release revision if needed or applicable

* Install dependencies if required
```
yum install gcc slurm-devel libconfig git
```

* Build the RPMs, defining the path to slurm libraries and slurm version if required
```
cd $HOME/rpmbuild/SPECS
rpmbuild -ba --define='lib /usr/lib64/slurm' --define='slurm_version slurm-19-05-4-1' ticrypt-spank.spec
```

* Install generated RPMs
```
yum install $HOME/rpmbuild/RPMS/x86_64/ticrypt-spank-<major>.<minor>-<release>.x86_64.rpm
```

### Configuration

The plugin utilizes a configuration file at /etc/ticrypt-spank.conf. For more information refer to 

```
man ticrypt-spank
```

As well as the annotated example configuration in 

```
ticrypt-spank/config/ticrypt-spank.conf
```

The example confiuration is set with the command /bin/true to allow quick testing of the plugin without any actual node re-configuration. 



### Testing

* After installing per the instructions on a single compute node, the plugin can be tested via srun run from that compute node. Exact options may change depending on the Slurm environment as well as the configuration options set for ticrypt-spank


```
srun --ntasks=1 --time=00:5:00 --ticrypt --pty /bin/bash -i
```

* After the plugin is installed on the Slurm contoller(s) it can be utilized in sbatch. A simple example is provided in this repository, but may need to be modified depending on the Slurm installation and ticrypt-spank settings.

```
sbatch ticrypt-spank/test/example.srun
```



## Authors

* **William Howell (whowell@rc.ufl.edu)** - *Initial work* 


## License

This project is licensed under the GNU GPLv3 License - see the [LICENSE.md](LICENSE.md) file for details


