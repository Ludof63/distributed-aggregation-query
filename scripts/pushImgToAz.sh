#PUSH THE IMAGES TO AZ
#requires the images to be built (./buildImages.sh)
#example
#./pushImgToAz.sh

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

if [[ ! -v REGISTRY_NAME ]]; then
    echo "REGISTRY_NAME is not set in config.sh" && exit 1
elif [[ -z "$REGISTRY_NAME" ]]; then
    echo "REGISTRY_NAME is set to the empty string in config.sh" && exit 1
fi

#REQUIRES AZ LOGIN -------------------------
az account show > /dev/null
if [ $? -ne 0 ]; then
    echo "AZ login required" && exit 1
fi

#auxiliar functions -----------------------
push_cordinator_image() {
    docker tag $COORD_IMAGE $REGISTRY_NAME.azurecr.io/$COORD_IMAGE
    docker push $REGISTRY_NAME.azurecr.io/$COORD_IMAGE
}

push_worker_image(){
    docker tag $WORKER_IMAGE $REGISTRY_NAME.azurecr.io/$WORKER_IMAGE
    docker push $REGISTRY_NAME.azurecr.io/$WORKER_IMAGE
}

docker login "$REGISTRY_NAME.azurecr.io"

#pushing images on AZ
push_cordinator_image &
pid_task1=$!

push_worker_image &
pid_task2=$!

#waiting for work to be done
wait $pid_task1
echo "Coordinator image pushed"

wait $pid_task2
echo "Worker image pushed"

#listing available image on AZ
echo "Available images: $(az acr repository list --name $REGISTRY_NAME)"


