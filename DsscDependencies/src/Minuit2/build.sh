rm -rf Minuit2-5.34.14/
tar xzf Minuit2-5.34.14.tar.gz
cd Minuit2-5.34.14
./configure --prefix=$(pwd)/Minuit-build
make -j
make install
