pids=""
for i in {1..10}
do
    ./cliente 127.0.0.1 < input.txt &
    pids="$pids $!"
done

for pid in $pids; do
    wait $pid
done