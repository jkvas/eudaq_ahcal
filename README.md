# eudaq_ahcal
CALICE ahcal modules to be built against the latest eudaq 

Script for combined out-of source build:
```bash
source ~/opt/root/6.22.08_src/install/bin/thisroot.sh

# temporary folder to store source code from git
EUDAQ_SOURCE=eudaq_official
ALTEL_SOURCE=eudaq_altel
AHCAL_SOURCE=eudaq_ahcal
ROOT_DIR=`pwd`
# temporary folder for building
BUILD_DIR=`pwd`/build
# final installation
INSTALL_DIR=`pwd`/install

git clone https://www.github.com/eudaq/eudaq $EUDAQ_SOURCE
pushd $EUDAQ_SOURCE
git checkout master #v2.4.4
popd

git clone https://github.com/eyiliu/altel_eudaq $ALTEL_SOURCE
pushd $ALTEL_SOURCE
git checkout build_gcc48 #or master?
popd

git clone https://github.com/jkvas/eudaq $AHCAL_SOURCE
pushd $AHCAL_SOURCE
git checkout Testbeam_202008
popd

mkdir -pv ${BUILD_DIR}/${EUDAQ_SOURCE}
pushd ${BUILD_DIR}/${EUDAQ_SOURCE}
cmake  -DBUILD_GUI=ON  \
       -DEUDAQ_BUILD_STDEVENT_MONITOR=ON  \
       -DEUDAQ_BUILD_ONLINE_ROOT_MONITOR=ON \
       -DEUDAQ_LIBRARY_BUILD_LCIO=ON \
       -DUSER_EUDET_BUILD=ON \
       -DUSER_TLU_BUILD=ON \
       -DUSER_EAMPLE_BUILD=ON \
       -DUSER_TIMEPIX3_BUILD=ON \
       -DUSER_CARIBOU_BUILD=ON \
       -DEUDAQ_INSTALL_PREFIX=${INSTALL_DIR} \
       -DPeary_DIR=/home/kvas/git/peary/share/cmake/Modules \
       -L ${ROOT_DIR}/${EUDAQ_SOURCE}
make  install
popd

mkdir -pv ${BUILD_DIR}/${ALTEL_SOURCE}
pushd ${BUILD_DIR}/${ALTEL_SOURCE}
cmake -L \
      -Deudaq_DIR=${INSTALL_DIR}/cmake \
      -DALTEL_BUILD_EUDAQ_MODULE=ON \
      -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}  \
      ${ROOT_DIR}/${ALTEL_SOURCE}
make  install
popd

mkdir -pv ${BUILD_DIR}/${AHCAL_SOURCE}
pushd ${BUILD_DIR}/${AHCAL_SOURCE}
cmake -L -Deudaq_DIR=${INSTALL_DIR}/cmake \
      -DDESYTABLE_BUILD_EUDAQ_MODULE=ON \
      -DUSER_CALICE_BUILD_AHCAL=ON \
      -DUSER_CALICE_BUILD=ON \
      -DUSER_CALICE_BUILD_BIF=ON \
      -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}  \
      ${ROOT_DIR}/${AHCAL_SOURCE}
make  install
popd
```
