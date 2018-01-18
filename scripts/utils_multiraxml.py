
import os
import sys
import shutil
import subprocess

def replacePrefix(commandFile, outputPath):
  lines = open(commandFile).readlines()
  with open(commandFile, "w") as f:
    for line in lines:
      split = line.split(" ")
      for i in range(0, len(split)):
        if split[i] == "--prefix":
          base = os.path.basename(split[i+1])
          split[i+1] = os.path.join(outputPath, base)
        f.write(split[i] + " ")

def getCommand(executable, commandFile, threads, outputPath):
  cmd = []
  cmd.append("mpirun")
  cmd.append("-np")
  cmd.append(str(threads))
  cmd.append(executable)
  cmd.append(commandFile)
  cmd.append(os.path.join(outputPath, "profile.svg"))
  return cmd

def run(executable, commandFile, threads, outputPath):
  cmd = getCommand(executable, commandFile, threads, outputPath)
  outputLog = os.path.join(outputPath, "logs.txt")
  print("running " + ' '.join(cmd))
  print("output will be printed in " + outputLog)
  with open(outputLog, "w") as outfile:
    subprocess.call(cmd, stdout=outfile)
  print("end of the run")
  
def duplicate(initialCommandFile, threads, outputPath):
  os.mkdir(outputPath)
  print("Created directory" + outputPath)
  commandFile = os.path.join(outputPath, os.path.basename(initialCommandFile))
  shutil.copyfile(initialCommandFile, commandFile)
  print("New command file: " + commandFile)
  replacePrefix(commandFile, os.path.join(outputPath, "raxmlrresults"))
  return commandFile
  
def duplicateAndRun(initialCommandFile, threads, outputPath):
  commandFile = duplicate(initialCommandFile, threads, outputPath)
  os.chdir(os.path.dirname(os.path.realpath(__file__)))
  executable = "../bin/raxml-ng-mpi"
  run(executable, commandFile, threads, outputPath)


