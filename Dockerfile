FROM nvidia/cuda:11.8.0-cudnn8-devel-ubuntu20.04
# FROM nvidia/cuda:11.7.1-cudnn8-devel-ubuntu20.04
# FROM nvidia/cuda:10.2-cudnn8-devel-ubuntu18.04


ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=Europe/Paris
RUN apt-get update && apt-get upgrade -y &&\
    # Install build tools, build dependencies and python
    apt-get install -y \
        build-essential \
        cmake \
        vim \
        git \
        wget \
        unzip \
        yasm \
        pkg-config \
        libswscale-dev \
        libtbb2 \
        libtbb-dev \
        libjpeg-dev \
        libpng-dev \
        libtiff-dev \
        libavformat-dev \
        libpq-dev \
        libxine2-dev \
        libglew-dev \
        libtiff5-dev \
        zlib1g-dev \
        libjpeg-dev \
        libavcodec-dev \
        libavformat-dev \
        libavutil-dev \
        libpostproc-dev \
        libswscale-dev \
        libeigen3-dev \
        libtbb-dev \
        libgtk2.0-dev \
        libssl1.1 \
        protobuf-compiler \
        libprotobuf-dev \
        libopenblas-dev \
        liblapack-dev \
        pkg-config &&\
    rm -rf /var/lib/apt/lists/*


        

ARG OPENCV_VERSION=4.6.0
ARG NPROC=16

RUN cd /opt/ &&\
    # Download and unzip OpenCV and opencv_contrib and delte zip files
    wget https://github.com/opencv/opencv/archive/$OPENCV_VERSION.zip &&\
    unzip $OPENCV_VERSION.zip &&\
    rm $OPENCV_VERSION.zip &&\
    wget https://github.com/opencv/opencv_contrib/archive/$OPENCV_VERSION.zip &&\
    unzip ${OPENCV_VERSION}.zip &&\
    rm ${OPENCV_VERSION}.zip &&\
    # Create build folder and switch to it
    mkdir /opt/opencv-${OPENCV_VERSION}/build && cd /opt/opencv-${OPENCV_VERSION}/build &&\
    # Cmake configure
    cmake \
        -DOPENCV_EXTRA_MODULES_PATH=/opt/opencv_contrib-${OPENCV_VERSION}/modules \
        -DWITH_CUDA=ON \
        -DCMAKE_BUILD_TYPE=RELEASE \
        # Install path will be /usr/local/lib (lib is implicit)
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        .. &&\
    # Make
    # make -j"$(nproc)" && \
    make -j$NPROC && \
    # Install to /usr/local/lib
    make install && \
    ldconfig && \
    # Remove OpenCV sources and build folder
    rm -rf /opt/opencv-${OPENCV_VERSION} && rm -rf /opt/opencv_contrib-${OPENCV_VERSION}

RUN apt-get update && apt-get install -y libepoxy-dev

RUN apt-get update && apt-get install -y \
    python3-dev

RUN cd /opt &&\
    git clone https://github.com/stevenlovegrove/Pangolin.git pangolin &&\
    cd pangolin &&\
    mkdir build &&\
    cd build &&\
    cmake \
        -DCMAKE_BUILD_TYPE=RELEASE \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        .. &&\
    make -j$NPROC &&\
    make install


RUN cd /opt &&\
    git clone https://github.com/davisking/dlib.git  dlib &&\
    cd dlib &&\
    mkdir build &&\
    cd build &&\
    cmake \
        -DCMAKE_BUILD_TYPE=RELEASE \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        .. &&\
    make -j$NPROC &&\
    make install

RUN cd /opt/ &&\
    git clone https://gitlab.inria.fr/tangram/oa-slam.git OA-SLAM &&\
    cd OA-SLAM &&\
    cd Thirdparty &&\
    cd DBoW2 &&\
    mkdir build &&\
    cd build &&\
    cmake .. &&\
    make -j$NPROC &&\
    cd ../.. &&\
    cd g2o &&\
    mkdir build &&\
    cd build &&\
    cmake .. &&\
    make -j$NPROC &&\
    cd ../.. &&\
    cd Osmap &&\
    sh build.sh &&\
    cd ../.. &&\
    mkdir build &&\
    cd build &&\
    cmake .. &&\
    make -j$NPROC &&\
    cd ../Vocabulary &&\
    tar -xvf ORBvoc.txt.tar.gz &&\
    ldconfig
