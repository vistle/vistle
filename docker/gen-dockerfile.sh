#! /bin/bash

case $1 in
   node)
      variant=node
      ;;
   frontend|*)
      variant=frontend
      ;;
esac

export UBUNTU=16.04
export PAR=-j4
export ISPCVER=1.9.1
export EMBREETAG=v2.15.0
export BUILDTYPE=Release
export PREFIX=/usr
export BUILDDIR=/build

case $UBUNTU in
   15.10)
      export ARCHSUFFIX=werewolfopt ;;
   16.04)
      export ARCHSUFFIX=xenialopt ;;
   *)
      export ARCHSUFFIX=linux64 ;;
esac

cat <<EOF
FROM library/ubuntu:${UBUNTU}
MAINTAINER "Martin Aumüller" <aumueller@hlrs.de>

WORKDIR ${BUILDDIR}

RUN apt-get update -y && apt-get install --no-install-recommends -y \
       libtbb-dev \
       libmpich-dev mpich \
       git cmake make file \
       libjpeg-dev \
       libvncserver-dev \
       libsnappy-dev zlib1g-dev libreadline-dev \
       libassimp-dev \
       libboost-atomic-dev libboost-date-time-dev libboost-exception-dev libboost-filesystem-dev \
       libboost-iostreams-dev libboost-locale-dev libboost-log-dev libboost-math-dev libboost-program-options-dev libboost-python-dev \
       libboost-random-dev libboost-serialization-dev libboost-system-dev libboost-thread-dev libboost-timer-dev \
       libboost-tools-dev libboost-dev

# dependencies for OpenGL/UI components
#RUN apt-get install --no-install-recommends -y libxmu-dev libxi-dev \
#       libopenscenegraph-dev libglew-dev \
#       qttools5-dev qtscript5-dev libqt5scripttools5 libqt5svg5-dev libqt5opengl5-dev libqt5webkit5-dev \

# https://bugs.launchpad.net/ubuntu/+source/libjpeg-turbo/+bug/1369067
RUN apt-get install --no-install-recommends -y libturbojpeg && cd /usr/lib/x86_64-linux-gnu && ln -s libturbojpeg.so.0 libturbojpeg.so

# for mpirun
RUN apt-get install --no-install-recommends -y openssh-server && mkdir -p /var/run/sshd

# install ispc - prerequisite for embree
RUN apt-get install --no-install-recommends -y wget ca-certificates \
       && cd /tmp \
       && wget http://sourceforge.net/projects/ispcmirror/files/v${ISPCVER}/ispc-v${ISPCVER}-linux.tar.gz/download \
       && tar -C /usr/bin -x -f /tmp/download --strip-components=1 ispc-v${ISPCVER}-linux/ispc \
       && rm download \
       && apt-get remove -y wget ca-certificates \
       && apt-get clean -y
EOF

# build embree CPU ray tracer
case $EMBREETAG in
   v2.9.0|v2.13.0|v2.15.0)
cat <<EOF-embree-2.9
ADD embree-debian-multiarch-${EMBREETAG}.diff ${BUILDDIR}/embree-debian-multiarch.diff
RUN git clone git://github.com/embree/embree.git && cd embree && git checkout ${EMBREETAG} \
      && git apply ../embree-debian-multiarch.diff \
      && rm ../embree-debian-multiarch.diff \
      && mkdir build && cd build \
      && cmake -DCMAKE_BUILD_TYPE=${BUILDTYPE} -DCMAKE_INSTALL_PREFIX=${PREFIX} -DENABLE_TUTORIALS=OFF -DEMBREE_TUTORIALS=OFF .. \
      && make ${PAR} install \
      && cd ${BUILDDIR} \
      && rm -rf embree
EOF-embree-2.9
      ;;
   v2.*)
cat <<EOF-embree-2.10
# build embree CPU ray tracer
RUN git clone git://github.com/embree/embree.git && cd embree && git checkout ${EMBREETAG} \
      && mkdir build && cd build \
      && cmake -DCMAKE_BUILD_TYPE=${BUILDTYPE} -DCMAKE_INSTALL_PREFIX=${PREFIX} -DENABLE_TUTORIALS=OFF -DEMBREE_TUTORIALS=OFF .. \
      && make ${PAR} install \
      && cd ${BUILDDIR} \
      && rm -rf embree
EOF-embree-2.10
      ;;
esac

cat <<EOF
# build COVISE file I/O library
RUN git clone git://github.com/hlrs-vis/covise.git \
       && export ARCHSUFFIX=${ARCHSUFFIX} \
       && export COVISEDIR=${BUILDDIR}/covise \
       && cd ${BUILDDIR}/covise \
       && mkdir -p build.covise \
       && cd build.covise \
       && cmake .. -DCOVISE_CPU_ARCH=corei7 -DCOVISE_BUILD_ONLY_FILE=TRUE -DCMAKE_INSTALL_PREFIX=${PREFIX} -DCMAKE_BUILD_TYPE=${BUILDTYPE} -DCOVISE_WARNING_IS_ERROR=FALSE \
       && make ${PAR} install \
       && cd ${BUILDDIR} \
       && rm -rf covise

# Vistle proper
RUN git clone --recursive git://github.com/vistle/vistle.git \
       && export ARCHSUFFIX=${ARCHSUFFIX} \
       && export COVISEDESTDIR=${BUILDDIR}/covise \
       && cd ${BUILDDIR}/vistle \
       && mkdir build.vistle \
       && cd build.vistle \
       && cmake -DVISTLE_CPU_ARCH=corei7 -DCMAKE_INSTALL_PREFIX=${PREFIX} -DICET_USE_OPENGL=OFF -DENABLE_INSTALLER=FALSE -DCMAKE_BUILD_TYPE=${BUILDTYPE} .. \
       && make ${PAR} VERBOSE=1 install \
       && cd ${BUILDDIR} \
       && rm -rf vistle
EOF

case $variant in
   frontend)
      echo EXPOSE 22 31093 31094 31590
      echo ENTRYPOINT [\"/usr/bin/vistle\"]
      echo CMD [\"-b\"]
      ;;
   node)
      echo EXPOSE 22 31094
      echo ENTRYPOINT [\"/usr/bin/vistle\"]
      echo CMD [\"-b\"]
      ;;
esac
