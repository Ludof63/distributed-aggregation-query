#EXECUTE COMPUTATION IN DOCKER WITH n_workers WORKERS and n_subpartitions SUBPARTITION FOR THE FIRST PHASE
#requires the images to be built (./buildImages.sh)
#example
#./computeDocker.sh 5 7

#ARGUMENTS VALIDATION-----------------------------------
if [ "$#" -ne 2 ]; then
  echo "Usage: $(basename "$0")  <n_of_workers> <n_subpartitions>"
  exit 1
fi

if [ "$1" -lt 1 ]; then
    echo "\"$1\": Number of worker must be grater than zero" && exit 1
fi

if [ "$2" -lt 1 ]; then
    echo "\"$1\": Number of subpartitions must be grater than zero" && exit 1
fi


#CONFIGURABLE PARAMETERS  from config.sh -------------------------
source $(pwd)/config.sh

if [[ ! -v COORD_IMAGE ]]; then
    echo "COORD_IMAGE is not set in config.sh" && exit 1
elif [[ -z "$COORD_IMAGE" ]]; then
    echo "COORD_IMAGE is set to the empty string in config.sh" && exit 1
fi

if [[ ! -v WORKER_IMAGE ]]; then
    echo "WORKER_IMAGE is not set in config.sh" && exit 1
elif [[ -z "$WORKER_IMAGE" ]]; then
    echo "WORKER_IMAGE is set to the empty string in config.sh" && exit 1
fi

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


#RUNNING ------------------------------
COORDINATOR="coordinator"
WORKER="worker"
PORT=4242

docker rm $COORDINATOR
docker run -d --network=cbdp_net --name=$COORDINATOR -e ACCOUNT=$STORAGE_ACCOUNT -e TOKEN=$TOK -e AZ_CONT=$SOURCE_CONTAINTER -e N_SUBPART=$2 $COORD_IMAGE
pid_coord=$!

sleep 1
for ((i=1; i<=$1; i++)); do
    docker rm $WORKER$i
    docker run -d --name=$WORKER$i --network=cbdp_net  -e CBDP_COORDINATOR=$COORDINATOR -e ACCOUNT=$STORAGE_ACCOUNT -e TOKEN=$TOK $WORKER_IMAGE
done

#following coordinator output
docker logs -f $COORDINATOR

wait $pid_coord
echo "Coordinator is done...saving result"

docker logs -n 25 $COORDINATOR > out/result.txt

#checking result correcteness
diff expected.txt out/result.txt &> /dev/null
if [ $? -eq 0 ]; then
    echo "Expected result matched" && exit 0
else
    echo "Expected result not matched" && exit 1
fi
