apk update

adduser -S -H -s /sbin/nologin minecraftspeedproxy

addgroup minecraftspeedproxy

apk add --no-cache -U make gcc pcre-dev bzip2-dev openssl-dev elogind-dev libc-dev dahdi-tools dahdi-tools-dev libexecinfo libexecinfo-dev ncurses-dev zlib-dev zlib

cd /tmp/

make

./minecraftspeedproxy --version

cp ./minecraftspeedproxy /usr/sbin

apk del gcc make

rm -rf /tmp/
