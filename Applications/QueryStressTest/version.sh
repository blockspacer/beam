#!/bin/bash
set -o errexit
set -o pipefail
source="${BASH_SOURCE[0]}"
while [ -h "$source" ]; do
  dir="$(cd -P "$(dirname "$source")" >/dev/null 2>&1 && pwd)"
  source="$(readlink "$source")"
  [[ $source != /* ]] && source="$dir/$source"
done
directory="$(cd -P "$(dirname "$source" )" >/dev/null 2>&1 && pwd)"
printf "#define QUERY_STRESS_TEST_VERSION \""> Version.hpp
git --git-dir=$directory/../../.git rev-list --count --first-parent HEAD | tr -d "\n" >> Version.hpp
printf \" >> Version.hpp
printf "\n" >> Version.hpp
