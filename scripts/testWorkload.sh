#EXECUTE WORKLOAD TEST OF COMPUTATION WITH n_workers WORKERS and n_subpartitions SUBPARTITION FOR THE FIRST PHASE
#requires executables to be ready (cd cmake-build-debug && make worker coordinator)
#example
#./testWorkload.sh 5 7

#ARGUMENTS VALIDATION-----------------------------------
set -euo pipefail

if [ "$#" -ne 2 ]; then
  echo "Usage: $(basename "$0") <n_of_workers> <n_subpartitions>"
  exit 1
fi

if [ "$1" -lt 1 ]; then
    echo "\"$1\": n_workers cannot be less than one" && exit 1
fi

if [ "$2" -lt 1 ]; then
    echo "\"$1\": n_subpartitions cannot be less than one" && exit 1
fi

#CONFIGURABLE PARAMETERS  from config.sh -------------------------
source $(pwd)/config.sh

if [[ ! -v SOURCE_CONTAINTER ]]; then
    echo "SOURCE_CONTAINTER is not set in config.sh" && exit 1
elif [[ -z "$SOURCE_CONTAINTER" ]]; then
    echo "SOURCE_CONTAINTER is set to the empty string in config.sh" && exit 1
fi

if [[ ! -v STORAGE_ACCOUNT ]]; then
    echo "STORAGE_ACCOUNT is not set config.sh" && exit 1
elif [[ -z "$STORAGE_ACCOUNT" ]]; then
    echo "STORAGE_ACCOUNT is set to the empty string config.sh" && exit 1
fi

#REQUIRES AZ LOGIN -------------------------
az account show > /dev/null
if [ $? -ne 0 ]; then
    echo "AZ login required" && exit 1
fi

#CREDENTIALS RETRIEVING--------------------------
TOK=$(az account get-access-token --resource https://storage.azure.com/ -o tsv --query accessToken)


#OTHER PARAMETERS-------------------------------
PORT="4242"

# Monitor the IO read activity of a command
traceIO() {
  touch "$1"
  TRACE="$1" "${@:2}"
}

amountOfWorker=$1


mkdir -p out

# Spawn the coordinator process
COORD_IP="localhost"
../cmake-build-debug/coordinator $PORT $STORAGE_ACCOUNT $TOK $SOURCE_CONTAINTER "c" $2 > out/coord_trace 2>&1 &
pid_coord=$!
echo "coordinator started"

sleep 1
# Spawn some workers
for i in $(seq 1 $amountOfWorker); do
  ../cmake-build-debug/worker $COORD_IP $PORT $STORAGE_ACCOUNT $TOK > "out/trace$i.txt" 2>&1 &
  echo "worker$i started"
done

# And wait for completion
time wait
echo "Coordinator is done"

# Then check how much work each worker did
echo ""
avg_aggr=$(expr 100 / $amountOfWorker)
avg_merg=$(expr $2 / $amountOfWorker)
for i in $(seq 1 $amountOfWorker); do
    agg_done=$(grep -c '^TAKS_DONE_a' "out/trace$i.txt")
    mrg_done=$(grep -c '^TAKS_DONE_m' "out/trace$i.txt")

    echo "Worker $i has done $agg_done aggregation. Average is $avg_aggr."
    echo "Worker $i has done $mrg_done merge. Average is $avg_merg."
    echo ""
done

#checking result correcteness
tail -n 25 out/coord_trace > out/result.txt
diff expected.txt out/result.txt &> /dev/null
if [ $? -eq 0 ]; then
    echo "Expected result matched" && exit 0
else
    echo "Expected result not matched" && exit 1
fi


for i in $(seq 1 $amountOfWorker); do rm "out/trace$i.txt"; done
rm out/coord_trace.txt