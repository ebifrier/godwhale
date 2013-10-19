while :
do
./bonanza <<EOF
stdout off
hash 22
mnj 15 100 hostname port# Weak1 0.1 16
EOF
echo Restart Bonanza
sleep 10
done
