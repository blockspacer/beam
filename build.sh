#!/bin/bash
source="${BASH_SOURCE[0]}"
while [ -h "$source" ]; do
  dir="$(cd -P "$(dirname "$source")" >/dev/null 2>&1 && pwd)"
  source="$(readlink "$source")"
  [[ $source != /* ]] && source="$dir/$source"
done
directory="$(cd -P "$(dirname "$source" )" >/dev/null 2>&1 && pwd)"

build_function() {
  if [ ! -d "$1" ]; then
    mkdir -p "$1"
  fi
  pushd "$1"
  $directory/$1/build.sh "$@"
  popd
}

export -f build_function
export directory

targets="Beam"
targets+=" Applications/AdminClient"
targets+=" Applications/ClientTemplate"
targets+=" Applications/DataStoreProfiler"
targets+=" Applications/HttpFileServer"
targets+=" Applications/QueryStressTest"
targets+=" Applications/RegistryServer"
targets+=" Applications/ServiceLocator"
targets+=" Applications/ServiceProtocolProfiler"
targets+=" Applications/ServletTemplate"
targets+=" Applications/UidServer"
targets+=" Applications/WebSocketEchoServer"

let cores="`grep -c "processor" < /proc/cpuinfo` / 2 + 1"
let mem="`grep -oP "MemTotal: +\K([[:digit:]]+)(?=.*)" < /proc/meminfo` / 4194304"
let jobs="$(($cores<$mem?$cores:$mem))"

parallel -j$jobs --no-notice build_function ::: $targets
