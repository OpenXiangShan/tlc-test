export CPHOME="/nfs/home/chenxi/CoupledL2"

C=0
B=0
M=0
R=0

# !! must follow cbmr order
while getopts "cbmr" OPT; do
  case $OPT in
    c) C=1;;
    b) B=1;;
    m) M=1;;
    r) R=1;;
    \?)
      echo "Unknown Option: -$OPTARG"
      echo "-c: compile, make test-top-xsconfig in CoupledL2"
      echo "-b: build,   cmake .. in build"
      echo "-m: make,    make -j in build"
      echo "-r: test run, ./build/tlc_test -c 200000"
      exit 1
      ;;
  esac
done

if [ $C -eq 1 ]; then
    make -C $CPHOME test-top-xsconfig
fi
if [ $B -eq 1 ]; then
    rm -rf build
    mkdir build
    cd build
    cmake .. -DDUT_DIR=$CPHOME/build -DTHREAD=4 -DTRACE=1 -DCHISELDB=1 -DCMAKE_BUILD_TYPE=Release -DNOCHECK=1
    cd ..
fi
if [ $M -eq 1 ]; then
    cd build
    make -j
    cd ..
fi
if [ $R -eq 1 ]; then
    ./build/tlc_test -c 200000
fi