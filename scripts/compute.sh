#EXECUTE COMPUTATION WITH n_workers WORKERS and n_subpartitions SUBPARTITION FOR THE FIRST PHASE
#requires executables to be ready (cd cmake-build-debug && make worker coordinator)
#example
#./compute.sh 5 7

#ARGUMENTS VALIDATION-----------------------------------
if [ "$#" -ne 2 ]; then
  echo "Usage: $(basename "$0") <n_workers> <n_subpartitions>" && exit 1
fi

#(0 doesn't make sense...please don't use it...used to test only final merge)
if [ "$1" -lt 0 ]; then
    echo "\"$1\": n_workers cannot be negative" && exit 1
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

#MAIN SCRIPT------------------------------------
#logs folder
mkdir -p out

#spwaning coordinator
COORD_IP="localhost"

../cmake-build-debug/coordinator $PORT $STORAGE_ACCOUNT $TOK $SOURCE_CONTAINTER "c" $2 > out/coordinator.txt 2>&1 &
pid_coord=$!

echo "Coordinator started"

sleep 1
#spwaning workers
for i in $(seq 1 $1); do
    echo "Starting worker$i"
    ../cmake-build-debug/worker $COORD_IP $PORT $STORAGE_ACCOUNT $TOK > out/worker$i.txt 2>&1 &
done

#waiting coordinator to be done to save result
wait $pid_coord
echo "Coordinator is done...saving result"
tail -n 25 out/coordinator.txt > out/result.txt

#checking result correcteness
diff expected.txt out/result.txt &> /dev/null
if [ $? -eq 0 ]; then
    echo "Expected result matched" && exit 0
else
    echo "Expected result not matched" && exit 1
fi









