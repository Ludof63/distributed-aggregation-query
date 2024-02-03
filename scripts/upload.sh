#EXECUTE UPLOAD FROM url TO container (can be specified in config file) USING n_workers WORKERS 
#example:
#./upload.sh 5 cbdp-5 https://db.in.tum.de/teaching/ws2324/clouddataprocessing/data/filelist.csv

#ARGUMENTS VALIDATION-----------------------------------
if [ "$#" -lt 2  ]; then
  echo "Usage: $(basename "$0") <n_workers> <url> <container ? optional>" && exit 1
fi

if [ "$#" -gt 3  ]; then
  echo "Usage: $(basename "$0") <n_workers> <url> <container ? optional>" && exit 1
fi

if [ "$1" -lt 1 ]; then
    echo "\"$1\": n_workers cannot be less than one" && exit 1
fi



#CONFIGURABLE PARAMETERS from config.sh  -------------------------
source $(pwd)/config.sh
if [[ ! -v STORAGE_ACCOUNT ]]; then
    echo "STORAGE_ACCOUNT is not set config.sh" && exit 1
elif [[ -z "$STORAGE_ACCOUNT" ]]; then
    echo "STORAGE_ACCOUNT is set to the empty string config.sh" && exit 1
fi


#CHOOSING WHICH CONTAINER TO USE -------------------------------
if [ "$#" -eq  3 ]; then
  echo "Using specified container"
  CONT=$3
else
  #using defaul container in config.sh
  if [[ ! -v SOURCE_CONTAINTER ]]; then
    echo "SOURCE_CONTAINTER is not set config.sh" && exit 1
  elif [[ -z "$SOURCE_CONTAINTER" ]]; then
      echo "SOURCE_CONTAINTER is set to the empty string config.sh" && exit 1
  fi

  echo "Using container from config: $SOURCE_CONTAINTER"
  CONT=$SOURCE_CONTAINTER
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

../cmake-build-debug/coordinator $PORT $STORAGE_ACCOUNT $TOK $CONT "u" $3 > out/coordinator.txt 2>&1 &
pid_coord=$!

echo "Coordinator started"
sleep 1

#spawning workers
for i in $(seq 1 $1); do
    echo "Starting worker$i"
    ../cmake-build-debug/worker $COORD_IP $PORT $STORAGE_ACCOUNT $TOK > out/worker$i.txt 2>&1 &
done

#waiting for coordinator to be done
wait $pid_coord
echo "Coordinator is done...upload complete"

exit 0