#!/bin/bash

# Exit on error
set -e

# Configuration
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$( cd "$SCRIPT_DIR/.." && pwd )"
PLUGIN_DIR="${PROJECT_ROOT}/packages/stride-plugin-intellij"
DIST_DIR="${PROJECT_ROOT}/dist"

echo "Building IntelliJ Plugin..."

# Ensure we are in the right directory to run gradlew
cd "${PLUGIN_DIR}"

# Run the buildPlugin task
./gradlew :generateStrideParser :generateStrideLexer :buildPlugin

# Create dist directory at root
mkdir -p "${DIST_DIR}"

# Find the generated zip file
# Usually in build/distributions/
ZIP_FILE=$(find build/distributions -name "*.zip" | head -n 1)

if [ -z "$ZIP_FILE" ]; then
    echo "Error: Could not find the generated plugin zip file."
    exit 1
fi

# Copy to dist directory
cp "$ZIP_FILE" "${DIST_DIR}/"
FILENAME=$(basename "$ZIP_FILE")

echo "--------------------------------------------------"
echo "Successfully built IntelliJ Plugin!"
echo "Plugin bundle: ${DIST_DIR}/${FILENAME}"
echo "You can now install this zip in IntelliJ via:"
echo "Settings > Plugins > ⚙️ > Install Plugin from Disk..."
echo "--------------------------------------------------"
