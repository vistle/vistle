FROM library/ubuntu:15.10
MAINTAINER "Martin Aumüller" <aumueller@hlrs.de>

WORKDIR /build
ADD embree-debian-multiarch.diff /build/embree-debian-multiarch.diff

RUN apt-get update -y && apt-get install --no-install-recommends -y        libtbb-dev        libmpich-dev mpich        git cmake make file        libjpeg-dev        libvncserver-dev        libsnappy-dev zlib1g-dev libreadline-dev        libassimp-dev        libboost-atomic-dev libboost-chrono-dev libboost-date-time-dev libboost-exception-dev libboost-filesystem-dev        libboost-iostreams-dev libboost-locale-dev libboost-log-dev libboost-math-dev libboost-program-options-dev libboost-python-dev        libboost-random-dev libboost-regex-dev libboost-serialization-dev libboost-system-dev libboost-thread-dev libboost-timer-dev        libboost-tools-dev libboost-dev

# dependencies for OpenGL/UI components
#RUN apt-get install --no-install-recommends -y libxmu-dev libxi-dev #       libopenscenegraph-dev libglew-dev #       qttools5-dev qtscript5-dev libqt5scripttools5 libqt5svg5-dev libqt5opengl5-dev libqt5webkit5-dev 
# https://bugs.launchpad.net/ubuntu/+source/libjpeg-turbo/+bug/1369067
RUN apt-get install --no-install-recommends -y libturbojpeg && cd /usr/lib/x86_64-linux-gnu && ln -s libturbojpeg.so.0 libturbojpeg.so

# install ispc - prerequisite for embree
RUN apt-get install --no-install-recommends -y wget ca-certificates        && cd /tmp        && wget http://sourceforge.net/projects/ispcmirror/files/v1.9.0/ispc-v1.9.0-linux.tar.gz/download        && tar -C /usr/bin -x -f /tmp/download --strip-components=1 ispc-v1.9.0-linux/ispc        && rm download        && apt-get remove -y wget ca-certificates        && apt-get clean -y

# build embree CPU ray tracer
RUN git clone git://github.com/embree/embree.git && cd embree && git checkout v2.9.0       && git apply ../embree-debian-multiarch.diff       && rm ../embree-debian-multiarch.diff       && mkdir build && cd build       && cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr -DENABLE_TUTORIALS=OFF ..       && make -j8 install       && cd /build       && rm -rf embree

# build COVISE file I/O library
RUN git clone git://github.com/hlrs-vis/covise.git        && export ARCHSUFFIX=werewolfopt        && export COVISEDIR=/build/covise        && cd /build/covise        && mkdir -p build.covise        && cd build.covise        && cmake .. -DCOVISE_NATIVE_ARCH=OFF -DCOVISE_BUILD_ONLY_FILE=TRUE -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release -DCOVISE_WARNING_IS_ERROR=FALSE        && make -j8 install        && cd /build        && rm -rf covise

# Vistle proper
RUN git clone --recursive git://github.com/vistle/vistle.git        && export ARCHSUFFIX=werewolfopt        && export COVISEDESTDIR=/build/covise        && cd /build/vistle        && mkdir build.vistle        && cd build.vistle        && cmake -DVISTLE_NATIVE_ARCH=OFF -DCMAKE_INSTALL_PREFIX=/usr -DICET_USE_OPENGL=OFF -DENABLE_INSTALLER=FALSE -DCMAKE_BUILD_TYPE=Release ..        && make -j8 install        && cd /build        && rm -rf vistle
EXPOSE 31093 31094 31590
ENTRYPOINT ["/usr/bin/vistle"]
CMD ["-b"]
