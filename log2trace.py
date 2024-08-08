import sqlite3
import argparse

def parse_arguments():
    parser = argparse.ArgumentParser(description="解析日志文件的脚本")
    parser.add_argument('-p', '--path', type=str, required=True, help='日志文件的路径')
    parser.add_argument('-o', '--output', type=str, required=True, help='输出文件的路径')
    return parser.parse_args()

args = parse_arguments()

# Connect to the database file
conn = sqlite3.connect(f'{args.path}')

# Create a cursor object to execute SQL queries
cursor = conn.cursor()

# Execute the SQL query
cursor.execute(
    "SELECT STAMP, SITE, CHANNEL, OPCODE, ADDRESS, PARAM FROM TLLog "
    "where (SITE='L2_L1D_0' or SITE='L2_PTW_0' or SITE='L2_L1I_0') "
    "and ("
    "   (CHANNEL=0 and OPCODE=4) or " # Get
    "   (CHANNEL=0 and OPCODE=6) or " # AcquireBlock # !TODO: add Hint
    "   (CHANNEL=0 and OPCODE=7) or " # AcquirePerm
    "   (CHANNEL=2 and OPCODE=6) or " # Release
    "   (CHANNEL=2 and OPCODE=7) "    # ReleaseData
    ") "
    "limit 1000"
)

# Fetch all the rows returned by the query
rows = cursor.fetchall()

traces = []
# Iterate over the rows and print the values
for row in rows:
    stamp, site, channel, opcode, address, param = row
    # print(f"STAMP: {stamp}, SITE: {site}, CHANNEL: {channel}, OPCODE: {opcode}, ADDRESS: {address}, PARAM: {param}")

    if site == 'L2_L1D_0':
        site = 1    # id = 1 for CAgent
    else:
        site = 0    # id = 0 for ULAgent

    # print(f"{stamp},{site},{channel},{opcode},{address},{param}")
    traces.append(f"{stamp},{site},{channel},{opcode},{address},{param}\n")

# Close the cursor and the connection
cursor.close()
conn.close()

with open(args.output, 'w') as f:
    f.writelines(traces)