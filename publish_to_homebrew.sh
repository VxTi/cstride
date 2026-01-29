#!/bin/bash

# Exit on error
set -e

# Configuration
PROJECT_ROOT=$(pwd)
BUILD_DIR="${PROJECT_ROOT}/packages/compiler/cmake-build-debug"
BINARY_NAME="cstride"
PACKAGE_NAME="stride"
VERSION="0.1.0" # You might want to automate versioning
DIST_DIR="${PROJECT_ROOT}/dist"
TARBALL_NAME="${PACKAGE_NAME}-${VERSION}.tar.gz"

# Public URL (optional - if provided, the formula will use this instead of a local file path)
# Usage: PUBLIC_URL=https://github.com/vxti/cstride/releases/download/v0.1.0/stride-0.1.0.tar.gz ./publish_to_homebrew.sh
DOWNLOAD_URL=${PUBLIC_URL:-"file://${DIST_DIR}/${TARBALL_NAME}"}

echo "Building ${BINARY_NAME}..."
cmake --build "${BUILD_DIR}" --target "${BINARY_NAME}"

# Create dist directory
mkdir -p "${DIST_DIR}"

# Prepare temporary staging area
STAGING_DIR=$(mktemp -d)
mkdir -p "${STAGING_DIR}/bin"
mkdir -p "${STAGING_DIR}/lib/stride"

# Copy binary
cp "${BUILD_DIR}/${BINARY_NAME}" "${STAGING_DIR}/bin/${BINARY_NAME}"

# Copy standard library if it exists
if [ -d "${PROJECT_ROOT}/packages/standard-library" ]; then
    echo "Packaging standard library..."
    cp -r "${PROJECT_ROOT}/packages/standard-library/"* "${STAGING_DIR}/lib/stride/"
fi

# Create tarball
echo "Creating tarball ${TARBALL_NAME}..."
tar -czf "${DIST_DIR}/${TARBALL_NAME}" -C "${STAGING_DIR}" .

# Calculate SHA256
SHA256=$(shasum -a 256 "${DIST_DIR}/${TARBALL_NAME}" | awk '{ print $1 }')
echo "SHA256: ${SHA256}"

# Generate Homebrew Formula
FORMULA_PATH="${DIST_DIR}/${PACKAGE_NAME}.rb"
TAP_DIR="/opt/homebrew/Library/Taps/vxti/homebrew-stride"
TAP_FORMULA_PATH="${TAP_DIR}/Formula/${PACKAGE_NAME}.rb"

echo "Generating Homebrew formula at ${FORMULA_PATH}..."

cat <<EOF > "${FORMULA_PATH}"
class Stride < Formula
  desc "Compiler for the Stride programming language"
  homepage "https://github.com/vxti/cstride"
  url "${DOWNLOAD_URL}"
  sha256 "${SHA256}"

  def install
    bin.install "bin/${BINARY_NAME}"
    (lib/"stride").install Dir["lib/stride/*"]
  end

  test do
    assert_match "Usage:", shell_output("#{bin}/${BINARY_NAME} --help")
  end
end
EOF

# Copy to local tap
if [ -d "${TAP_DIR}" ]; then
    echo "Copying formula to local tap ${TAP_DIR}..."
    mkdir -p "${TAP_DIR}/Formula"
    cp "${FORMULA_PATH}" "${TAP_FORMULA_PATH}"
    
    echo "Installing ${PACKAGE_NAME} from local tap..."
    brew uninstall "${PACKAGE_NAME}" || true
    brew install "${PACKAGE_NAME}"
    
    echo "Verifying installation..."
    if ${BINARY_NAME} --help | grep -q "Usage:"; then
        echo "Successfully published and installed ${PACKAGE_NAME}!"
    else
        echo "Installation verification failed."
        exit 1
    fi
else
    echo "Local tap directory ${TAP_DIR} not found. Skipping local installation."
    echo "To publish, upload the tarball to your release page and update the URL/SHA256 in the formula."
fi

# Cleanup staging
rm -rf "${STAGING_DIR}"

echo "Done! Tarball and Formula are in ${DIST_DIR}"
