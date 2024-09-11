# we expect no check for data correctness,
# therefore for RAM in L2/L3 DataStorage and TLRAM,
# we just want it to output zero

import argparse
import os
parser = argparse.ArgumentParser()
parser.add_argument('-s', type=str, help='CPHOME')
args = parser.parse_args()
cphome = args.s

# ! TODO: these targets require manual check
# they are [L3-data-ram, L2-data-ram, Memory]
targets = ['array_3_ext.v', 'array_mcp2_ext.v', 'mem_ext.v']
depth = [65536, 4096, 2147483648]
width = [64, 512, 256]

files = [os.path.join(cphome, 'build', target) for target in targets]

for (i, file) in enumerate(files):
    lines = []
    with open(file, 'r') as f:
        while True:
            line = f.readline()
            lines.append(line)
            if line.startswith(')'):
                break

    line = lines[-2]
    # print(line)
    data_width = int(line.split('[')[1].split(':')[0]) + 1
    assert(data_width == width[i])
    # print(data_width)

    lines.append(f"  assign RW0_rdata = {data_width}'b0;\n")
    lines.append(f"endmodule")

    with open(file, 'w') as f:
        f.writelines(lines)
        print(f"set {file} as zero ram")
