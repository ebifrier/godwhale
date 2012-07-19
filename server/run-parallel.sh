while :
do
perl parallel_server.pl --split_nodes 300000 --server_id Parallel1 --client_port 4084
sleep 10
done
