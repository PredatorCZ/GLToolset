pushd imgui

if [ ! -d main ]; then
    git clone git@github.com:ocornut/imgui.git main
fi

pushd main
    git checkout docking
    git pull origin docking
popd

set -e

cp -f ./main/im* .
cp -f ./main/backends/im*opengl3* .
cp -f ./main/backends/im*glfw* .

rm -rdf main
popd
