#!/bin/dash

if [ -e debian/control ]; then
    echo "Debian control found"
    dir=$(pwd)
else
    echo "No Debian control found; not in the right directory?" >&2
    exit 1
fi

version=$(head -n 1 debian/changelog | egrep -o '[0-9]+\.[0-9]+\.[0-9]+')
major=$(echo $VERSION | awk -F. '{print $1}')
project=$(awk '/^Source:/ {print $2; exit}' debian/control)
echo "Project $project -- version is ${version} (major: ${major})."

make clean

echo "Starting release."

export TMPDIR=${TMPDIR:-${TMP:-/tmp}}
dpkg-buildflags | sed -r 's/^/export /;s/=(.*)$/="\1"/' > $TMPDIR/.flags$$
. $TMPDIR/.flags$$

export DEVSCRIPTS_CHECK_DIRNAME_LEVEL=0
export DEB_BUILD_HARDENING=1

exec make release
