#! /bin/bash

# should match output of root CMakeLists.txt

prog="$0"

fatal() {
   echo "$prog: $@" >&2
   exit 1
}

git=$(which git) 2>&1 || fatal "git not found"

VISTLE_VERSION_TAG="unknown"
VISTLE_VERSION_HASH="unknown"

VISTLE_VERSION_HASH=$("$git" log -n1 --format=%h/%ci | sed -e s',/.*$,,') || fatal "git log failed"
# for latest tag contained by hash
VISTLE_VERSION_TAG=$("$git" tag --sort=v:refname --merged | grep '^v20' | tail -n 1 | sed -e 's/^v//') || fatal "git tag failed"
# working copy clean/dirty?
GIT_STATUS_OUT=$("$git" status --untracked-files=no --porcelain) || fatal "git status failed"

test -n "$GIT_STATUS_OUT" && VISTLE_VERSION_HASH="$VISTLE_VERSION_HASH"+

echo $VISTLE_VERSION_TAG-$VISTLE_VERSION_HASH

exit 0
