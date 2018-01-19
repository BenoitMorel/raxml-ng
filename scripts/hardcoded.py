import cluster


def run(commandFile, outputPath, threads, naive = False):
  if naive:
    outputPath += "naive_"
  outputPath += str(threads)
  cluster.duplicateAndRun(commandFile, threads, outputPath, naive)

carine_command = "/home/morelbt/github/multi-raxml/deps/raxml-ng/runs/data/carine.txt"
carine_outputPath = "/home/morelbt/github/multi-raxml/deps/raxml-ng/runs/scripted_carine_"

example_command = "/home/morelbt/github/multi-raxml/deps/raxml-ng/runs/data/example.txt"
example_outputPath = "/home/morelbt/github/multi-raxml/deps/raxml-ng/runs/scripted_example_"

#run(example_command, example_outputPath, 16, True)
#run(example_command, example_outputPath, 16, False)
#run(example_command, example_outputPath, 32, True)
#run(example_command, example_outputPath, 32, False)

#run(carine_command, carine_outputPath, 128, True)
#run(carine_command, carine_outputPath, 128, False)

#run(carine_command, carine_outputPath, 256, True)
#run(carine_command, carine_outputPath, 256, False)

#run(carine_command, carine_outputPath, 512, True)
run(carine_command, carine_outputPath, 512, False)




