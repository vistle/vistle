FROM library/ubuntu:16.04
MAINTAINER "Martin Aumüller" <aumueller@hlrs.de>

WORKDIR /build

RUN apt-get update -y && apt-get install --no-install-recommends -y        libtbb-dev        libmpich-dev mpich        git cmake make file        libjpeg-dev        libvncserver-dev        libsnappy-dev zlib1g-dev libreadline-dev        libassimp-dev        libtinyxml2-dev libvtk6-dev        libboost-atomic-dev libboost-date-time-dev libboost-exception-dev libboost-filesystem-dev        libboost-iostreams-dev libboost-locale-dev libboost-log-dev libboost-math-dev libboost-program-options-dev libboost-python-dev        libboost-random-dev libboost-serialization-dev libboost-system-dev libboost-thread-dev libboost-timer-dev        libboost-tools-dev libboost-dev

# dependencies for OpenGL/UI components
#RUN apt-get install --no-install-recommends -y libxmu-dev libxi-dev #       libopenscenegraph-dev libglew-dev #       qttools5-dev qtscript5-dev libqt5scripttools5 libqt5svg5-dev libqt5opengl5-dev libqt5webkit5-dev 
RUN apt-get install --no-install-recommends -y libturbojpeg && cd /usr/lib/x86_64-linux-gnu && ln -s libturbojpeg.so.0 libturbojpeg.so
# for mpirun
RUN apt-get install --no-install-recommends -y dnsutils openssh-server && mkdir -p /var/run/sshd
RUN cd /root && ssh-keygen -q -t rsa -N "" -f /root/.ssh/id_rsa && cat /root/.ssh/id_rsa.pub >> /root/.ssh/authorized_keys
RUN echo 'root:screencast' | chpasswd
RUN sed -i 's/PermitRootLogin prohibit-password/PermitRootLogin yes/' /etc/ssh/sshd_config
# SSH login fix. Otherwise user is kicked off after login
RUN sed 's@session\s*required\s*pam_loginuid.so@session optional pam_loginuid.so@g' -i /etc/pam.d/sshd

# install ispc - prerequisite for embree
RUN apt-get install --no-install-recommends -y wget ca-certificates        && cd /tmp        && wget -q http://sourceforge.net/projects/ispcmirror/files/v1.9.2/ispc-v1.9.2-linux.tar.gz/download        && tar -C /usr/bin -x -f /tmp/download --strip-components=1 ispc-v1.9.2-linux/ispc        && rm download        && apt-get remove -y wget ca-certificates        && apt-get clean -y
ADD embree-debian-multiarch-v2.17.x.diff /build/embree-debian-multiarch.diff
RUN git clone git://github.com/embree/embree.git && cd embree && git checkout v2.17.4       && git apply ../embree-debian-multiarch.diff       && rm ../embree-debian-multiarch.diff       && mkdir build && cd build       && cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr -DENABLE_TUTORIALS=OFF -DEMBREE_TUTORIALS=OFF ..       && make -j16 install       && cd /build       && rm -rf embree
# build COVISE file I/O library
RUN git clone git://github.com/hlrs-vis/covise.git        && export ARCHSUFFIX=xenialopt        && export COVISEDIR=/build/covise        && cd /build/covise        && mkdir -p build.covise        && cd build.covise        && cmake .. -DCOVISE_CPU_ARCH=corei7 -DCOVISE_BUILD_ONLY_FILE=TRUE -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release -DCOVISE_WARNING_IS_ERROR=FALSE        && make -j16 install        && cd /build        && rm -rf covise

# Vistle proper
RUN git clone --recursive git://github.com/vistle/vistle.git        && export ARCHSUFFIX=xenialopt        && export COVISEDESTDIR=/build/covise        && cd /build/vistle        && mkdir build.vistle        && cd build.vistle        && cmake -DVISTLE_CPU_ARCH=corei7 -DCMAKE_INSTALL_PREFIX=/usr -DICET_USE_OPENGL=OFF -DENABLE_INSTALLER=FALSE -DCMAKE_BUILD_TYPE=Release ..        && make -j16 VERBOSE=1 install        && cd /build        && rm -rf vistle

ADD bin/spawn_vistle.sh /usr/bin/spawn_vistle.sh
ADD bin/make_hostlist.sh /root/bin/make_hostlist.sh
ADD bin/make_ssh_known_hosts.sh /root/bin/make_ssh_known_hosts.sh
ADD bin/mpirun.sh /root/bin/mpirun.sh
ADD bin/start.sh /root/bin/start.sh

EXPOSE 22 31093 31094 31590
CMD ["/root/bin/start.sh"]
