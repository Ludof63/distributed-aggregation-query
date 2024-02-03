#EXECUTE COMPUTATION ON AZURE WITH n_workers WORKERS and n_subpartitions SUBPARTITION FOR THE FIRST PHASE
#requires images to be pushed on AZ (./buildImages.sh && ./pushImgToAz.sh)
#example
#./computeAZ.sh 5 7

CPU=1 #TO USE MORE THAN FIVE WORKERS -> DESCREASE NUMBER OF VCPU FOR WORKER [LIMITED TO 6 FOR REGION for azure student (1 always for coordinator)]

#VALIDATION ARGUMENTS-----------------------------------
if [ "$#" -ne 2 ]; then
  echo "Usage: $(basename "$0") <n_workers> <n_subpartitions> " && exit 1
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

if [[ ! -v REGISTRY_NAME ]]; then
    echo "REGISTRY_NAME is not set in config.sh" && exit 1
elif [[ -z "$REGISTRY_NAME" ]]; then
    echo "REGISTRY_NAME is set to the empty string in config.sh" && exit 1
fi

if [[ ! -v RESOURCE_GROUP ]]; then
    echo "RESOURCE_GROUP is not set in config.sh" && exit 1
elif [[ -z "$RESOURCE_GROUP" ]]; then
    echo "RESOURCE_GROUP is set to the empty string in config.sh" && exit 1
fi

#REQUIRES AZ LOGIN
az account show > /dev/null
if [ $? -ne 0 ]; then
    echo "AZ login required" && exit 1
fi

#CREDENTIALS RETRIEVING--------------------------
REG_PSWD=$(az acr credential show --name $REGISTRY_NAME  -g $RESOURCE_GROUP --query 'passwords[0].value')
REG_PSWD=${REG_PSWD//\"/}

TOK=$(az account get-access-token --resource https://storage.azure.com/ -o tsv --query accessToken)

#other params used --------
NET=cbdpVnet
SUBNET=cbdpSubnet
COORDINATOR=coordinator
PORT=4242

#VNET-----------------------------------
az network vnet show -g $RESOURCE_GROUP --name $NET &> /dev/null
if [ $? -eq 0 ]; then
  echo "vnet $NET exists."
else
  echo "vnet $NET does not exist...creating it"
  az network vnet create -g $RESOURCE_GROUP --name $NET &> /dev/null
fi

#SUBNET---------------------------------------
az network vnet subnet show -g $RESOURCE_GROUP --vnet-name $NET -n $SUBNET &> /dev/null
if [ $? -eq 0 ]; then
  echo "subnet $SUBNET exists."
else
  echo "subnet $SUBNET does not exist...creating it"
  az network vnet subnet create -g $RESOURCE_GROUP --vnet-name $NET -n $SUBNET --address-prefixes 10.0.0.0/24
fi

#usage <container_name> <image> <env variables>
create_container(){
    local NAME="$1"
    local IMAGE="$2"
    local ENV_VARS="$3"

    echo "Creating container $NAME"

    az container create -g $RESOURCE_GROUP --vnet $NET --subnet $SUBNET --restart-policy Never --cpu $CPU --name $NAME --image "$REGISTRY_NAME.azurecr.io"/$IMAGE  -e $ENV_VARS --registry-username $REGISTRY_NAME --registry-password ${REG_PSWD} &> /dev/null
    if [ $? -ne 0 ]; then
        echo "failed to create container $NAME" && exit 1
    else
        echo "container with name $NAME created"
    fi 
}

remove_container(){
    local COONT_NAME="$1"
    az container delete -y -g $RESOURCE_GROUP --name $COONT_NAME &> /dev/null
    echo "container with name $COONT_NAME removed"
}

# first create the coordinator
create_container $COORDINATOR $COORD_IMAGE "ACCOUNT=$STORAGE_ACCOUNT TOKEN=$TOK AZ_CONT=$SOURCE_CONTAINTER N_SUBPART=$2" 

# then create the workers (in parallel)
COORDINATOR_IP=$(az container show -g $RESOURCE_GROUP --name $COORDINATOR --query ipAddress.ip --output tsv)
echo "$COORDINATOR ip: $COORDINATOR_IP"

echo "Creating workers"
workers_pid=()
for ((i = 1; i <= $1; i++)); do
    create_container "worker$i" $WORKER_IMAGE "CBDP_COORDINATOR=$COORDINATOR_IP ACCOUNT=$STORAGE_ACCOUNT TOKEN=$TOK" &
    workers_pid+=($!)
done

echo "Waiting for workers to be created"
for ((i = 1; i <= $1; i++)); do
    wait ${workers_pid[i]}
    echo "worker$i created"
done

echo "All workers are created...following coordinator"

az container logs -g $RESOURCE_GROUP --name $COORDINATOR --follow


echo "Saving result..."
az container logs -g $RESOURCE_GROUP --name $COORDINATOR | tail -n 31 > out/aux.txt
echo ""
cat <  out/aux.txt | head -n 4
echo ""
tail -n 26 out/aux.txt > out/result.txt
rm out/aux.txt

echo "Deleting AZ resources..."

for ((i = 1; i <= $1; i++)); do
    remove_container "worker$i"
done
remove_container $COORDINATOR

exit 0
