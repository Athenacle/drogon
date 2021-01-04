#!/usr/bin/env bash

function install_deps(){
    apt update
    apt install apt-transport-https curl make git g++ libjsoncpp-dev uuid-dev openssl libssl-dev \
        zlib1g-dev libsqlite3-dev libbrotli-dev lsb-release software-properties-common sudo psmisc -yy

    # https://www.postgresql.org/download/linux/ubuntu/
    echo "deb http://apt.postgresql.org/pub/repos/apt $(lsb_release -cs)-pgdg main"  | tee -a /etc/apt/sources.list.d/pgdg.list > /dev/null
    curl -Ls https://www.postgresql.org/media/keys/ACCC4CF8.asc | apt-key add 

    # https://apt.kitware.com/
    echo "deb https://apt.kitware.com/ubuntu/ $(lsb_release -cs)-rc main" | tee -a /etc/apt/sources.list.d/cmake.list > /dev/null
    curl -Ls https://apt.kitware.com/keys/kitware-archive-latest.asc | apt-key add

    add-apt-repository -y ppa:mhier/libboost-latest

    apt update
    apt install postgresql-server-dev-10 cmake boost1.67 -yy

    gcc --version
    cmake --version
    psql --version
    openssl version
}

function install_gtest(){
    curl -Ls https://github.com/google/googletest/archive/release-1.10.0.tar.gz | tar xz
    pushd googletest-release-1.10.0 || exit -1
    mkdir build
    cmake .
    make -j${nproc}
    make install
    popd
}

function doit(){
    pushd drogon/ || exit -1
    echo "=====================BEGIN============================="
    bash -ex build.sh -t
    bash -x test.sh -t
}

install_deps
install_gtest
doit