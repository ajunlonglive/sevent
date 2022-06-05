#clear && rm -rf ./build/* && cd ./build && cmake .. && make
#SOURCE_DIR=$(pwd) `pwd`
clear="clear"
if [[ $1 = $clear ]]
then
rm -rf ./build2/* && cd ./build2 && cmake .. && make
exit
fi

cd ./build2 && cmake .. && make