This is a port to MAC OS X or FreeBSD for testing.

Install on FreeBSD 15
sudo pkg install cmake git libdispatch
git clone https://github.com/bacnet-stack/bacnet-stack.git
cd bacnet-stack
mkdir build
cd build
cmake ..
make
