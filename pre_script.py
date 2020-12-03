import os
import glob
import shutil
import sys
import cogapp
from zipfile import ZipFile

# Compile Faust Code
err = os.system("faust2teensy -lib FaustInstrument.dsp")
if err:
    print(f"ERROR: failed to compile with faust2teensy (status code {err})", file=sys.stderr)
    sys.exit(1)

# Copy to now location
os.system("unzip FaustInstrument.zip")
with ZipFile('FaustInstrument.zip', 'r') as FaustZip:
    FaustZip.extractall()

for faustFile in glob.glob(f"FaustInstrument{os.sep}FaustInstrument.*"):
    shutil.copy(faustFile, "src")

# Generating JSON
err = os.system("faust -json FaustInstrument.dsp")
if err:
    print(f"ERROR: failed to generate json with faust -json (status code {err})", file=sys.stderr)
    sys.exit(1)

shutil.copy("FaustInstrument.dsp.json", "src")

# Running COG Code Genraten
cog = cogapp.Cog()
cog.processFile(f"src{os.sep}main.cpp", f"src{os.sep}main_cog.cpp")

os.remove(f"src{os.sep}main.cpp")
shutil.move(f"src{os.sep}main_cog.cpp", f"src{os.sep}main.cpp")

# Cleanup
os.remove("FaustInstrument.zip")
shutil.rmtree("FaustInstrument")
os.remove("FaustInstrument.dsp.json")
