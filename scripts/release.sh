mkdir -p deb/DEBIAN
mkdir -p deb/lib/systemd/system
mkdir -p deb/usr/bin
cp ./minecraftspeedproxy deb/usr/bin
tee deb/lib/systemd/system/Minecraft-Speed-Proxy.service <<-'EOF'
[Unit]
Description=Minecraft speedup program

[Service]
type=forking
ExecStart=sudo minecraftspeedproxy -c /etc/minecraftspeedproxy/config.json
EOF

cd deb/DEBIAN
touch control postinst postrm
chmod 755 control postinst postrm

tee control <<-'EOF'
Package: Minecraft-Speed-Proxy
EOF

echo Version: $VERSION >> control

tee control -a <<-'EOF'
Architecture: amd64
Maintainer: AllesUgo<yangpengxuanab@outlook.com>
Installed-Size: 1.08M
Provides: optional
Section: net
Description: Minecraft speedup program.
EOF

echo 'sudo systemctl daemon-reload' >> postinst
echo 'sudo systemctl daemon-reload' >> postrm

cd ../..
dpkg -b deb/ Minecraft-Speed-Proxy-linux-amd64.deb
