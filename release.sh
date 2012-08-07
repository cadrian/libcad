#!/bin/dash

dir=$(cd $(dirname $(readlink -f $0)); pwd)
cd $dir

version=$(head -n 1 Changelog | egrep -o '[0-9]+\.[0-9]+\.[0-9]+')
echo "Version is ${version}."

if grep -vq "^libcad ($version-1)" debian/changelog; then
    echo "Generating Debian changelog."
    mv debian/changelog debian/changelog~
    {
        echo "libcad ($version-1) unstable; urgency=low"
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
    tar=libyacjp_${version}.orig.tar.bz2
    exec tar cfj $tar --exclude-vcs $(basename $dir)
)

echo "Starting release."
exec make release
