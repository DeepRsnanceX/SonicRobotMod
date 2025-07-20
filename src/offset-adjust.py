import os
import plistlib
import re

# Hi! if you're reading this, chances are you're messing around with the code for my "Sonic Robot Mod" for Geode.
# If you are, i'm so sorry. This mod will get rewritten eventually bc it is so ass.
#
# That aside! If you've built the mod yourself after doing some changes and tested it in-game, you might notice
# the Sonic sprites are a lil too low, clipping into the ground. Basically, they're not where they should be, at all.
#
# This script fixes that. This is the script i made to automatically adjust the sprite positions AFTER building the mod,
# since i obviously, for one, can NOT ship the mod with fucked up offsets and will NOT bother adjusting everything manually.
#
# So, how does this work in case i, having edited the mod, need to fix my sheets? Here's its usage:
# - After the mod is built, download the .geode file and save it anywhere.
# - Open the .geode file as if it was a .zip file (Right click it > open with > WinRAR/7zip | Alternatively, right click > 7zip > open file)
# ^ (You can also just make a copy, rename it to .zip and open that)
# - Navigate to the resources folder and enter the folder found in it.
# - In here, locate all 3 RobotSheet's .PNG and .PLIST files. Take all of them (6 files total) and extract them somewhere. DON'T CLOSE THE .geode FILE YET!!!
# - Whenever you extracted the sheets, place this script in that same directory.
# - Then, simply run the script, either via CMD or simply double-clicking it (You will need Python installed afaik).
# - Once it's done running, place all 6 files (AKA, all 3 sheets) back onto the .geode file.
# - Save the changes to the file if prompted to.
# - Load the mod in-game. Sprites *should* be fixed in their correct position now!
# If they aren't, either something went wrong with the script, or you need to fix the positioning on the mod's resources before building.

# Y offset values
offsetHighY = 68
offsetMediumY = 34
offsetLowY = 17

# X offset values
offsetHighX = 4
offsetMediumX = 2
offsetLowX = 1

keywords = ["sonicIdle", "sonicBumped", "sonicDeath", "sonicRun"]

def getOffset(filename):
    if "-uhd" in filename:
        return offsetHighY, offsetHighX
    elif "-hd" in filename:
        return offsetMediumY, offsetMediumX
    else:
        return offsetLowY, offsetLowX

def modifyPlist(plistPath):
    with open(plistPath, 'rb') as file:
        plistData = plistlib.load(file)

    modified = False

    frames = plistData.get('frames', {})
    
    for key, value in frames.items():
        if isinstance(value, dict) and 'spriteOffset' in value:
            spriteOffset = value['spriteOffset']
            
            match = re.match(r'\{(-?\d+),(-?\d+)\}', spriteOffset)
            if match:
                xOffset, yOffset = int(match.group(1)), int(match.group(2))
                
                filenameOffsetY, filenameOffsetX = getOffset(os.path.basename(plistPath))
                
                if any(keyword in key for keyword in keywords):

                    newYOffset = yOffset + filenameOffsetY
                    
                    newXOffset = xOffset + filenameOffsetX
                    
                    value['spriteOffset'] = f'{{{newXOffset},{newYOffset}}}'
                    modified = True

    if modified:
        with open(plistPath, 'wb') as file:
            plistlib.dump(plistData, file)
        print(f'Modified: {plistPath}')
    else:
        print(f'No modifications needed: {plistPath}')

def main():
    currentDirectory = os.getcwd()
    
    for filename in os.listdir(currentDirectory):
        if filename.endswith('.plist'):
            modifyPlist(filename)

if __name__ == "__main__":
    main()
