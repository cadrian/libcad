#!/usr/bin/env bash

# Called by build tools (Travis...), don't use directly

echo "Building: $1"

cd $(dirname $(readlink -f $0))/..

cp -af build/debian.skel debian
make target/version
version=$(< target/version)
echo version=$version

for f in build/debian.$1/*; do
    sed "s/#VERSION#/$version/g" $f > debian/${f##*/}
done

rm -f debian/files
exec make release.$1
