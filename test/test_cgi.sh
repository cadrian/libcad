#!/usr/bin/env bash

HOME=$(pwd)
DIR=${TESTDIR:-$(mktemp --tmpdir -d test_cgi.XXXXXX)}
echo DIR=$DIR

CONF=$DIR/conf
ROOT=$DIR/root
LOG=$DIR/log
RUN=$DIR/run

mkdir -p $CONF $ROOT $LOG $RUN

cat > $CONF/lighttpd.conf <<EOF
server.chroot = "$DIR"
server.document-root = "root"
server.port = 8888
server.tag = "test_cgi"
server.modules = ("mod_cgi","mod_auth","mod_accesslog")

auth.backend = "plain"
auth.backend.plain.userfile = "conf/users"
auth.require = ( "/" => (
        "method" => "basic",
        "realm" => "Password protected area",
        "require" => "user=test"
    )
)

mimetype.assign = (
    ".html" => "text/html",
    ".txt"  => "text/plain",
    ".jpg"  => "image/jpeg",
    ".png"  => "image/png",
    ""      => "application/octet-stream"
)

static-file.exclude-extensions = ( ".fcgi", ".php", ".rb", "~", ".inc", ".cgi" )
index-file.names = ( "index.html" )

server.errorlog = "log/error.log"
server.breakagelog = "log/breakage.log"
accesslog.filename = "log/access.log"

\$HTTP["url"] =~ "^/test_cgi\.cgi" {
    cgi.assign = ( ".cgi" => "$(which bash)" )
}
EOF

cat > $CONF/users <<EOF
test:pwd
EOF

cat > $ROOT/index.html <<EOF
<html>
    <head>
        <meta http-equiv="refresh" content="0; url=/test_cgi.cgi">
    </head>
</html>
EOF

cat >$ROOT/test_cgi.cgi <<EOF
#!/bin/dash
export PATH=/bin:/usr/bin
export HOME=$DIR
export XDG_CACHE_HOME=$RUN
export XDG_RUNTIME_DIR=$RUN
export XDG_DATA_HOME=$RUN
export XDG_CONFIG_HOME=$CONF
export XDG_DATA_DIRS=$RUN
export XDG_CONFIG_DIRS=$RUN
{
    env | sort
    echo
    echo script is $HOME/$0
} > $LOG/env.log
exec $HOME/${0%.sh}.exe
EOF

(
    cd $DIR
    exec /usr/sbin/lighttpd -D -f $CONF/lighttpd.conf
) &
lighttpd_pid=$!

sleep 2
curl -m10 'http://test:pwd@localhost:8888/test_cgi.cgi?foo=bar'

kill $lighttpd_pid

cat $LOG/breakage.log

rm -rf $DIR
