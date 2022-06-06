#clear && rm -rf ./build/* && cd ./build && cmake .. && make
clear="clear"
build_dir="./build2"
cmake_dir=$(pwd)
install_dir=$cmake_dir
# clear path
if [[ $1 = $clear ]];then
rm -r $build_dir/*
fi
# cmake execute path
if [ ! -d $build_dir ];then
   mkdir -p $build_dir
fi
# cmake
cd $build_dir && \
cmake $cmake_dir \
    -DBUILD_EXAMPLES=OFF \
    -DBUILD_TESTS=OFF \
    -DBUILD_RELEASE=ON \
    -DENABLE_OPENSSL=ON \
    && \
make && \
make install