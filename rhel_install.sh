#!/bin/bash

if [ "$#" -ne 1 ]
then 
    echo "Usage $0 <install directory>"
    exit 1
fi

INSTALL_DIR=`readlink -f $1`

# First we get the needed yum packages
yum groupinstall "Development Tools"
yum install qt-devel git cmake libXmu-devel libXi-devel freeglut-devel boost-devel libxml2-devel apr-devel httpd-devel

mkdir install_tmp
cd install_tmp

# Download GLEW:
wget -O glew.tgz http://downloads.sourceforge.net/project/glew/glew/1.10.0/glew-1.10.0.tgz?r=http%3A%2F%2Fglew.sourceforge.net%2F&ts=1380204337&use_mirror=kent
tar xvf glew.tgz
cd glew-1.10.0
GLEW_DEST=${INSTALL_DIR} make install
cd ..

# Download GLM
wget -O glm.zip http://downloads.sourceforge.net/project/ogl-math/glm-0.9.4.6/glm-0.9.4.6.zip?r=http%3A%2F%2Fglm.g-truc.net%2F0.9.4%2Findex.html&ts=1380204611&use_mirror=kent
unzip glm.zip
cp -r glm/glm ${INSTALL_DIR}/include

# Download Tinia
git clone http://github.com/sintefmath/tinia.git
cd tinia
git checkout kjetilly_rhel_fixing
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} -DCMAKE_PREFIX_PATH=${INSTALL_DIR}
make -j9 install
cd ..
cd ..

# Download HPMC
git clone http://github.com/sintefmath/hpmc.git
cd hpmc
git checkout kjetilly_rhel_maintenance_fix
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} -DCMAKE_PREFIX_PATH=${INSTALL_DIR}
make -j9 install
cd ..
cd ..

# Download FRView
git clone http://github.com/sintefmath/FRView.git
cd FRView
git checkout kjetilly_rhel_fixing
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} -DCMAKE_PREFIX_PATH=${INSTALL_DIR}
make -j9 install
cd ..
cd ..

# Install Apache locally:
cp -r /etc/httpd ${INSTALL_DIR}/
rm -rf ${INSTALL_DIR}/httpd/log ${INSTALL_DIR}/httpd/run ${INSTALL_DIR}/httpd/modules
mkdir ${INSTALL_DIR}/httpd/log ${INSTALL_DIR}/httpd/run
ln -s /etc/httpd/modules ${INSTALL_DIR}/httpd/modules
sed -i "s/ServerRoot \/etc\/httpd/ServerRoot ${INSTALL_DIR}/httpd/g" ${INSTALL_DIR}/httpd/conf/httpd.conf
sed -i 's/Listen 80/Listen 8080/g' ${INSTALL_DIR}/httpd/conf/httpd.conf

TO_INSERT <<EOF
LoadModule trell_module ${INSTALL_DIR}/var/trell/module/libmod_trell.so
TrellMasterId "trell_master"
TrellMasterExe "${INSTALL_DIR}/var/trell/bin/trell_master"
TrellAppRoot "${INSTALL_DIR}/var/trell/apps"
TrellSchemaRoot "${INSTALL_DIR}/var/trell/schemas"
TrellJobWWWRoot "${INSTALL_DIR}/var/trell/js"

LogLevel notice

<Location /trell/mod>
  SetHandler trell
</Location>
<Location /trell/master>
  SetHandler trell
</Location>
<Location /trell/job>
  SetHandler trell
</Location>

Alias /trell/static ${INSTALL_DIR}/var/trell/static
<Directory "${INSTALL_DIR}/var/trell/static">
   Options -Indexes
   AllowOverride None
   Order allow,deny
   Allow from all
</Directory>
EOF

echo $TO_INSERT >> ${INSTALL_DIR}/httpd/conf/httpd.conf

echo <<EOF 
To run the httpd server, issue the command

   LD_LIBRARY_PATH=\$LD_LIBRARY_PATH:${INSTALL_DIR}/lib:${INSTALL_DIR}/lib64 httpd -d ${INSTALL_DIR}/httpd

EOF
