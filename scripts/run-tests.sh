 #!/bin/bash
 # Fail immediately if any command exits with a non-zero status.
 set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$( cd "$SCRIPT_DIR/.." && pwd )"

BLUE="\033[34m"
WHITE="\033[37m"
RESET="\033[0m"

printf "${BLUE}+---------------------------------------+${RESET}\n"
printf "${BLUE}|                                       |${RESET}\n"
printf "${BLUE}| ${WHITE}Running tests for Stride compiler...  ${BLUE}|${RESET}\n"
printf "${BLUE}|                                       |${RESET}\n"
printf "${BLUE}+---------------------------------------+${RESET}\n"

cmake --build "${PROJECT_ROOT}/packages/compiler/cmake-build-debug" --target cstride_tests && "${PROJECT_ROOT}/packages/compiler/cmake-build-debug/tests/cstride_tests"

printf "${BLUE}+----------------------------------------------+${RESET}\n"
printf "${BLUE}|                                              |${RESET}\n"
printf "${BLUE}| ${WHITE}Running tests for stride-plugin-intellij...  ${BLUE}|${RESET}\n"
printf "${BLUE}|                                              |${RESET}\n"
printf "${BLUE}+----------------------------------------------+${RESET}\n"

cd "${PROJECT_ROOT}/packages/stride-plugin-intellij"; ./gradlew clean test
