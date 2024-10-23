# bash-aptfile

a simple method of defining apt-get dependencies for an application

## requirements

- apt-get
- dpkg

## installation

```shell
# curl all the things!
curl -o /usr/local/bin/aptfile https://raw.githubusercontent.com/seatgeek/bash-aptfile/master/bin/aptfile
chmod +x /usr/local/bin/aptfile
```

You can *also* create a debian package via docker using `make` with the provided `Dockerfile`. The debian package is made via the wonders of `fpm`:

```shell
git clone https://github.com/seatgeek/bash-aptfile.git
cd bash-aptfile
make deb

# sudo all the things!
sudo dpkg -i *.deb
```

## usage

Create an `aptfile` in the base of your project:

```shell
#!/usr/bin/env aptfile
# ^ note the above shebang

# trigger an apt-get update
update

# install some packages
package "build-essential"
package "git-core"
package "software-properties-common"

# install a ppa
ppa "fkrull/deadsnakes-python2.7"

# install a few more packages from that ppa
package "python2.7"
package "python-pip"
package "python-dev"

# setup some debian configuration
debconf_selection "mysql mysql-server/root_password password root"
debconf_selection "mysql mysql-server/root_password_again password root"

# install another package
package "mysql-server"

# install a package from a download url
package_from_url "google-chrome-stable" "https://dl.google.com/linux/direct/google-chrome-stable_current_amd64.deb"

# you can also execute arbitrary bash
echo "🚀  ALL GOOD TO GO"
```

Now you can run it:

```shell
# aptfile *does not* use sudo by default!
sudo aptfile

# enable bash tracing
TRACE=1 sudo -E aptfile

# you can also execute a specific aptfile
sudo aptfile path/to/your/aptfile

# or make the file executable and run it directly
chmod +x path/to/your/aptfile
sudo ./aptfile
```

And you'll see the following lovely output:

```
Running update
[NEW] package build-essential
[NEW] package git-core
[NEW] package software-properties-common
[NEW] ppa fkrull/deadsnakes-python2.7
[NEW] package python2.7
[NEW] package python-pip
[NEW] package python-dev
[OK] set debconf line: mysql mysql-server/root_password password root
[OK] set debconf line: mysql mysql-server/root_password_again password root
[NEW] package mysql-server
[NEW] package google-chrome-stable
🚀 ALL GOOD TO GO
```

Note that `aptfile` runs uses `--force-confnew` - it will forcibly use the package's version of a conf file if a conflict is found.

## aptfile primitives

You can use any of the following primitives when creating your service's aptfile:

### update

Runs apt-get update:

```shell
update
```

### package

Installs a single package:

```shell
package "git-core"
```

### package_from_url

Installs a single package from a url:

```shell
package_from_url "google-chrome-stable" "https://dl.google.com/linux/direct/google-chrome-stable_current_amd64.deb"
```

### packagelist

Installs a multiple packages in one apt call:

```shell
packagelist "git-core" "gitsome"
```

### repository

Installs an aptitude repository via `add-apt-repository`:

```shell
repository "deb http://us.archive.ubuntu.com/ubuntu/ saucy universe multiverse"
repository "ppa:mozillateam/firefox-next"
```

### repository_file

Installs an aptitude repository in `/etc/apt/sources.list.d`:

```shell
# Add the exact line to /etc/apt/sources.list.d/google-chrome.list
repository_file "google-chrome" "deb http://dl.google.com/linux/chrome/deb/ stable main"
# without 'deb', suite (defaults to lsb_release -sc) and components (defaults to 'main') are added
# All three lines do the same (on xenial)
repository_file "git-lfs.list" "deb https://packagecloud.io/github/git-lfs/ubuntu/ xenial main"
repository_file "git-lfs.list" "https://packagecloud.io/github/git-lfs/ubuntu/ main"
repository_file "git-lfs.list" "https://packagecloud.io/github/git-lfs/ubuntu/"
```

### ppa

The preferred method for installing a ppa as it properly handles not re-running `add-apt-repository`:

```shell
ppa "mozillateam/firefox-next"
```

### debconf_selection

Allows you to set a debconf selection:

```shell
debconf_selection "mysql mysql-server/root_password password root"
```

## helper functions

These helper functions can be used inside your custom aptfile.

### log_fail

Logs a message to standard error and exits. If this is called, the full output from the dpkg calls will be output as well for further inspection.

```shell
log_fail "Unable to find the proper package version"
```

### log_info

Outputs a message to stdout.

```shell
log_info "🚀 ALL GOOD TO GO"
```
