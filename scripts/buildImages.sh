#BUILD DOCKER IMAGES NEEDED TO EXECUTE COMPUTATION
#example
#./buildImages.sh

#CONFIGURABLE PARAMETERS from config.sh -------------------------
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


#BUILDING THE IMAGES------------------------------
COORD_TARGET=coordinator_5
WORKER_TARGET=worker_5

docker build -t $COORD_IMAGE --target $COORD_TARGET ../
if [ $? -ne 0 ]; then
   echo "Failed to build $COORD_IMAGE image" && exit 1
fi
docker build -t $WORKER_IMAGE --target $WORKER_TARGET ../
if [ $? -ne 0 ]; then
   echo "Failed to build $WORKER_IMAGE image" && exit 1
fi
