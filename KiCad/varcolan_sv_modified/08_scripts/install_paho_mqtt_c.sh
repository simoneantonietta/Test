#!/bin/bash
#
# Installs the matching version of Paho MQTT C library required by the C++ lib.
#
#before run script pleas install
#
# apt-get install build-essential gcc make cmake cmake-gui cmake-curses-gui doxygen graphviz libssl-dev

git clone https://github.com/eclipse/paho.mqtt.c.git
pushd paho.mqtt.c
git checkout develop
mkdir build_cmake && cd build_cmake
#cmake -DPAHO_WITH_SSL=ON -DPAHO_BUILD_DOCUMENTATION=TRUE -DPAHO_BUILD_SAMPLES=TRUE -DPAHO_BUILD_STATIC=TRUE -DPAHO_MQTT_C_PATH=../../paho.mqtt.c ..
cmake -DCMAKE_CXX_COMPILER=g++ -DPAHO_WITH_SSL=ON -DPAHO_BUILD_DOCUMENTATION=TRUE -DPAHO_BUILD_SAMPLES=TRUE -DPAHO_BUILD_STATIC=TRUE -DPAHO_MQTT_C_PATH=../../paho.mqtt.c ..
make
sudo make install
sudo ldconfig
popd

exit 0


