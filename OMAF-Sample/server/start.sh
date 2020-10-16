#!/bin/bash -ex

DIR=$(dirname $(readlink -f "$0"))
NODEIP=$(kubectl get node -o wide | \
         grep -v master | \
         grep -oE "\b([0-9]{1,3}\.){3}[0-9]{1,3}\b")

for YAML in $(find "${DIR}" -maxdepth 1 -name "*.yaml" -print); do
    sed -i "s/NODEIP/${NODEIP}/g" ${YAML}
    kubectl apply -f "${YAML}"
done
