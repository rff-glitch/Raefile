#/usr/bin/bash
mkdir build 
cd build 
cmake ..
make 
sudo cp ./Raefile /usr/bin/raefile
raefile
