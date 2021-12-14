#!/usr/bin/env bash

export VISTLE_DOCUMENTATION_TARGET=$1
export VISTLE_DOCUMENTATION_DIR=$2
vistle --batch $3
