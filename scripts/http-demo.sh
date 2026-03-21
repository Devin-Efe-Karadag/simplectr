#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
exec ./bin/simplectr run --name web-demo --net bridge --publish 18080:8080 -- /bin/sh -ec '
cat > /root/reply.sh <<'"'"'EOF'"'"'
#!/bin/sh
while IFS= read -r line; do
    [ "$line" = "$(printf "\r")" ] && break
done
printf "HTTP/1.1 200 OK\r\nContent-Length: 10\r\nConnection: close\r\n\r\npublished\n"
EOF
chmod +x /root/reply.sh
echo READY
exec /bin/busybox nc -lk -p 8080 -e /root/reply.sh
'
