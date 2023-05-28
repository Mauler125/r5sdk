#------------------------------------------------------------------------------
import sys
import os
import re

s_filePath = os.getcwd()

#------------------------------------------------------------------------------
# Checks if this is the target level and renames it
#------------------------------------------------------------------------------
def RenameFileOnMatch(pattern, fileName, oldLevelName, newLevelName):
    match = re.match(pattern, fileName)
    if match:
        levelName = match.group(1)
        if levelName == oldLevelName:
            newFileName = f"{newLevelName}{match.group(2)}"
            os.rename(os.path.join(s_filePath, fileName),
                      os.path.join(s_filePath, newFileName))
            print(f"Renamed {fileName} to {newFileName}")

#------------------------------------------------------------------------------
# Renames the target BSP and its associated lump files
#------------------------------------------------------------------------------
def RenameBspFiles(oldLevelName, newLevelName):
    for fileName in os.listdir(s_filePath):

        # Rename BSP
        if fileName.endswith('.bsp'):
            pattern = r'^(.*?)(\.[^\.]+$)'
            RenameFileOnMatch(pattern, fileName, oldLevelName, newLevelName)

        # Rename lumps
        elif fileName.endswith('.bsp_lump'):
            pattern = r'^(.*?)(\..*?)$'
            RenameFileOnMatch(pattern, fileName, oldLevelName, newLevelName)

        # Rename entity partition
        elif fileName.endswith('.ent'):
            pattern = r'^(.*?)(_[^_]+$)'
            RenameFileOnMatch(pattern, fileName, oldLevelName, newLevelName)

#------------------------------------------------------------------------------
if __name__ == "__main__":
    if len(sys.argv) > 1:
        RenameBspFiles(sys.argv[1], sys.argv[2])
    else:
        print("Usage: ren_map.py <oldLevelName> <newLevelName>")
