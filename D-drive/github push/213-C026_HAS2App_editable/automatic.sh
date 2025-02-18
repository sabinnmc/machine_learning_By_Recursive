#!/bin/bash
# 1. running a make file  i.e."make" command 
# container name 
CONTAINER="container_jan20"
#check if the container existed and running
if docker ps -q -f name="container_jan20";then
	# runnig "make" file 
	docker exec -it "$CONTAINER" make
else
	echo "Error: container '$CONTAINER' is not running."
	exit 1
fi
# 2. Transfer binary file to HAS2
scp ./bin/Has2App has2@192.168.0.47:/home/share/has2app/


