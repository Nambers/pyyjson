.vscode/build.sh
mkdir -p build
cd build
rm ../python-test/pyyjson.so
cp ./pyyjson.so ../python-test/
