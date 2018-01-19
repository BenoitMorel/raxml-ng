import cluster

command = "/home/morelbt/github/multi-raxml/deps/raxml-ng/runs/data/carine.txt"
outputPath = "/home/morelbt/github/multi-raxml/deps/raxml-ng/runs/scripted_carine_"

threads = 128
cluster.duplicateAndRun(command, threads, outputPath + str(threads))

threads = 256
cluster.duplicateAndRun(command, threads, outputPath + str(threads))

threads = 512
cluster.duplicateAndRun(command, threads, outputPath + str(threads))




