# pearjs-experiment

``` sh
# expects v8-prebuilds to be in your $PATH
npm i -g holepunchto/v8-prebuilds

# fetch the submodules
git submodules update --init

# cmake it
cmake -S . -B build
make -C build

# and you are done
./build/bin/pear index.js
```
