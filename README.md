# ticrypt-spank

A [Slurm](https://www.schedmd.com/) SPANK plugin to allow calling cluster specific modules for node reconfiguration to [TiCrypt](https://terainsights.com/) VM hosts. The plugin adds the --ticrypt Slurm option for requesting a TiCrypt node. The plugin utilizes a configuration file to enforce limits or requirements on accounts, features, etc ... which are allowed to call the plugin.

## Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes. See deployment for notes on how to deploy the project on a live system.

### Prerequisites

This SPANK plugin requires [libconfig](http://hyperrealm.github.io/libconfig/) which is broadly availble under common Linux distributions. Utilizing the plugin requires a functional [Slurm](https://www.schedmd.com/) installation with [plugstack](https://slurm.schedmd.com/spank.html) configuration. Building the packages or development requires slurm-devel. 

### Installation

#### Manual Installation

* Clone the repository

* Copy the example configuration to system

```
cp config/ticrypt-spank.conf
``` 

* Build the plugin optionally specifying DESTDIR if required for your environment

```
# example with Slurm installed in /opt/slurm

cd ticrypt-spank/
make install DESTDIR=/opt/slurm
```

#### RPM Build and Install
* Setup a build environment per system requirements such as [CentOS](https://wiki.centos.org/HowTos/SetupRpmBuildEnvironment)

* In the steps below, <major> should be substitued by the desired major release, and <minor> substituted by the desired minor release

* Download desired release to SOURCES
```
cd $HOME/rpmbuild/SOURCES
wget https://github.com/UFResearchComputing/ticrypt-spank/archive/<major>-<minor>.tar.gz
```

* Depending on the git system downloaded from, it may be necessary to extract the source, rename, and compress for example
```
cd $HOME/rpmbuild/SOURCES
tar -xf <major>-<minor>.tar.gz
mv ticrypt-spank-<major>-<minor> ticrypt-spank-<major>
tar -czf ticrypt-spank-v<major>.<minor>.tar.gz ticrypt-spank-<major>
```

* Copy the specfile from the source to SPECS
```
cd $HOME/rpmbuild/SOURCES
tar -xf <major>-<minor>.tar.gz   #depend on if renamed above
cp ticrypt-spank-<major>/ticrypt-spank.spec $HOME/rpmbuild/SPECS
```

* Install dependencies if required
```
yum install gcc slurm-devel libconfig
```

* Build the RPMs
```
cd $HOME/rpmbuild/SPECS
rpmbuild -ba ticrypt-spank.spec
```

* Install generated RPMs
```
yum install $HOME/rpmbuild/RPMS/x86_64/ticrypt-spank-<major>-<minor>.x86_64.rpm
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


