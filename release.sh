#!/bin/dash

dir=$(cd $(dirname $(readlink -f $0)); pwd)
cd $dir

version=$(head -n 1 Changelog | egrep -o '[0-9]+\.[0-9]+\.[0-9]+')
major=$(echo $version | egrep -o '^[0-9]+')
project=$(awk '/^Source:/ {print $2; exit}' debian/control)
echo "Project $project -- version is ${version} (major: ${major})."

if grep -q "^$project ($version-1)" debian/changelog; then
    echo "Debian changelog is up to date."
else
    echo "Generating Debian changelog."
    mv debian/changelog debian/changelog~
    {
        echo "$project ($version-1) unstable; urgency=low"
        echo
        echo "  * Release from upstream $version"
        echo
        echo " -- Cyril ADRIAN <cyril.adrian@gmail.com>  $(date -R)"
        echo
        cat debian/changelog~
    } > debian/changelog
fi

make clean

echo "Saving orig."
(
    cd ..
    tar=${project}_${version}.orig.tar.bz2
    exec tar cfj $tar --exclude-vcs $(basename $dir)
)

echo "Starting release."

export TMPDIR=${TMPDIR:-${TMP:-/tmp}}
dpkg-buildflags | sed -r 's/^/export /;s/=(.*)$/="\1"/' > $TMPDIR/.flags$$
. $TMPDIR/.flags$$

export DEVSCRIPTS_CHECK_DIRNAME_LEVEL=0
export DEB_BUILD_HARDENING=1

exec make release
