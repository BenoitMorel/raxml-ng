import cluster

command = "/home/morelbt/github/multi-raxml/deps/raxml-ng/runs/data/multiple.txt"
threads = 4
outputPath = "/home/morelbt/github/multi-raxml/deps/raxml-ng/runs/tmp"
cluster.duplicateAndRun(command, threads, outputPath)
