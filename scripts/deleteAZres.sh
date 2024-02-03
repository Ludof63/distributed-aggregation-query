RESOURCE_GROUP=cbdp-resourcegroup
COORDINATOR=coordinator


if [ "$#" -ne 1 ]; then
  echo "Usage: $(basename "$0") <n_workers>" && exit 1
fi

if [ "$1" -lt 0 ]; then
    echo "\"$1\": n_workers cannot be neagative" && exit 1
fi

remove_container(){
    local COONT_NAME="$1"
    az container delete -y -g $RESOURCE_GROUP --name $COONT_NAME &> /dev/null
    echo "container with name $COONT_NAME removed"
}


for ((i = 1; i <= $1; i++)); do
    remove_container "worker$i"
done
remove_container $COORDINATOR