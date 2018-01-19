

import sys
import os
import subprocess
import shutil
import utils_multiraxml

def duplicateAndRun(initialCommandFile, threads, outputPath):
  try:
    shutil.rmtree(outputPath)
  except:
    pass
  commandFile = utils_multiraxml.duplicate(initialCommandFile, threads, outputPath)
  submitFile = os.path.join(outputPath, "submit.sh")

  mypath = os.path.dirname(os.path.realpath(__file__))
  executable = os.path.join(mypath, "../bin/raxml-ng-mpi")
  command = " ".join(utils_multiraxml.getCommand(executable, commandFile, threads, outputPath)) 
  print("Command to run: " + command) 
  nodes = str((int(threads) - 1) // 16 + 1)
  with open(submitFile, "w") as f:
    f.write("#!/bin/bash\n")
    f.write("#SBATCH -o " + os.path.join(outputPath, "myjob.out" ) + "\n")
    f.write("#SBATCH -B 2:8:1\n")
    f.write("#SBATCH -N " + str(nodes) + "\n")
    f.write("#SBATCH -n " + str(threads) + "\n")
    f.write("#SBATCH --threads-per-core=1\n")
    f.write("#SBATCH --cpus-per-task=1\n")
    f.write("#SBATCH --hint=compute_bound\n")
    f.write("#SBATCH -t 24:00:00\n")
    f.write("\n")
    f.write(command)
  print("submit file: " + submitFile)
  subprocess.check_call(["sbatch", "-s" ,submitFile])



  
