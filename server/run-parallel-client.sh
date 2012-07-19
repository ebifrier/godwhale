while :
do
./bonanza <<EOF
stdout off
hash 26
tlp num 12
dfpn_client hostname_dfpn_server port#_dfpn_server
mnj 15 1 hostname_majority_server port#_majority_or_parallel_server Client1-1 1.0
EOF
echo Restart Bonanza
sleep 10
done
