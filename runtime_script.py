import subprocess
import math

cluster_counts = [2, 3, 4, 5, 6]
point_counts = [100000, 200000, 300000, 400000, 500000]

for c in cluster_counts:
    for p in point_counts:
        minimum = math.inf
        cmd = ["mpiexec", "./p5", str(c), str(p)]
        output_str = subprocess \
            .run(cmd, stdout=subprocess.PIPE).stdout.decode('utf-8')
        time = float(output_str)
        minimum = min(minimum, time)
        print(f"{minimum},", end="")
    print()
