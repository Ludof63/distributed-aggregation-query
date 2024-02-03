#A CONFIGURATION FILE TO RUN THE SCRIPTS IN THIS FOLDER

#storage account name
STORAGE_ACCOUNT=cbdp62storage

#container of the data (100 chunks)
SOURCE_CONTAINTER=cbdp-5

#name of the images (for docker and AZ tests (only compute))
COORD_IMAGE=cbdp_coordinator_5
WORKER_IMAGE=cbdp_worker_5

#needed to push images to Az (./pushImgToAz.sh) and to compute on AZ (./computeAz.sh)
REGISTRY_NAME=cbdpgroup62

#needed to to compute on AZ (./computeAz.sh)
RESOURCE_GROUP=cbdp-resourcegroup