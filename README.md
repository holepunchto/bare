# pearjs-experiment

``` sh
# install deps
npm install

# fetch v8 prebuild from the p2p network
npm run fetch-v8

# fetch the submodules
git submodules update --init

# cmake it
cmake -S . -B build
make -C build

# and you are done
./build/bin/pear index.js
```
