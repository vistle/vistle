#!/bin/bash

NETWORK_FILE="$1"
OUT_DIR="$2"
SRC_DIR="$3"
REF_HASH="$4"
VISTLE_DIR="$5"

export SNAPSHOT_ARGS="-name $NETWORK_FILE -o $OUT_DIR -sdir $SRC_DIR"
export COCONFIG=${VISTLE_DIR}/doc/config.vistle.doc.xml

vistle $VISTLE_DIR/doc/resultSnapShot.py 

python3 $VISTLE_DIR/test/moduleTest/utils/createImageHash.py -name $NETWORK_FILE -sdir ${OUT_DIR}/${NETWORK_FILE}_result.png -cmp -refHashString ${REF_HASH} 

if [ $? != 0 ]; then
    exit 1
fi

exit 0