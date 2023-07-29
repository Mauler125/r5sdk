#------------------------------------------------------------------------------
import os
import json
import hashlib

#------------------------------------------------------------------------------
# Compute the SHA-256 checksum of a file
#------------------------------------------------------------------------------
def ComputeChecksum(filePath, blockSize=65536):
    checksum = hashlib.sha256()
    with open(filePath, "rb") as file:
        for block in iter(lambda: file.read(blockSize), b""):
            checksum.update(block)
    return checksum.hexdigest()

#------------------------------------------------------------------------------
# Compute checksums for all files in a directory
#------------------------------------------------------------------------------
def RecursiveComputeChecksum(directoryPath):
    checksums = {}
    scriptPath = os.path.abspath(__file__)

    for root, _, files in os.walk(directoryPath):
        for fileName in files:
            filePath = os.path.join(root, fileName)
            relativePath = os.path.relpath(filePath, directoryPath)
            normalizedPath = relativePath.replace("\\", "/")

            # Exclude the script itself from the manifest
            if os.path.abspath(filePath) == scriptPath:
                continue

            checksum = ComputeChecksum(filePath)
            checksums[normalizedPath] = checksum

    return checksums

#------------------------------------------------------------------------------
# Save the checksums to a manifest file
#------------------------------------------------------------------------------
def CreateManifest(checksums, outManifestFile):
    with open(outManifestFile, "w") as jsonFile:
        json.dump(checksums, jsonFile, indent=4)

#------------------------------------------------------------------------------
if __name__ == "__main__":
    workingDirectory = os.getcwd()
    outManifestFile = "manifest.json"

    checksums = RecursiveComputeChecksum(workingDirectory)
    CreateManifest(checksums, outManifestFile)
