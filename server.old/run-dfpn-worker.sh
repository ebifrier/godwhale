while :
do
./bonanza <<EOF
dfpn connect hostname port#
EOF
echo Restart worker
sleep 10
done
