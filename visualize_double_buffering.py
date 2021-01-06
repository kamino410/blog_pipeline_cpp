import sys
import re
from io import StringIO
import numpy as np
import matplotlib.pyplot as plt

datastr = sys.stdin.read()
pattern = re.compile(r'<([^<>]+)>\s*(.+)\s*</\1>', re.MULTILINE | re.DOTALL)
valuelist = re.findall(pattern, datastr)
valuedict = dict(valuelist)

print("Keys : " + ', '.join([key for key, val in valuelist]))

costs = np.loadtxt(StringIO(valuedict['costs']), delimiter=',')
data1 = np.loadtxt(StringIO(valuedict['th1']), delimiter=',')
data2 = np.loadtxt(StringIO(valuedict['th2']), delimiter=',')

costs = costs.astype(np.uint16)
data1 = np.array([row for row in data1 if row[0] != -1 and row[4] < 15000])
data2 = np.array([row for row in data2 if row[0] != -1 and row[3] < 15000])

plt.figure(figsize=(18, 3))

plt.hlines(np.ones(len(data1))*0, data1[:, 1],
           data1[:, 2], colors='#96ceb4', lw=30)
plt.hlines(np.ones(len(data1))*0,
           data1[:, 3], data1[:, 4], colors='#d9534f', lw=30)
plt.hlines(np.ones(len(data2))*1,
           data2[:, 1], data2[:, 2], colors='#ffad60', lw=30)
plt.hlines(np.ones(len(data2))*1,
           data2[:, 2], data2[:, 3], colors='#ffeead', lw=30)

plt.vlines(data1[:, 4], 0.2, 0.8, linestyles='dotted', colors='gray')

for row in data1:
    plt.text(row[3]+100, 0.3, str(int(row[0])))

for row in data2:
    plt.text(row[3]+100, 1.3, str(int(row[0])))

plt.title('Double buffering (A: '+str(costs[0])+', B: '+str(costs[1]) +
          ', C: '+str(costs[2])+', D: '+str(costs[3])+')')
plt.ylim(1.5, -0.5)
plt.xlabel('time [μs]')
plt.yticks(np.arange(2), ['Thread1', 'Thread2'])

plt.savefig("result.png", bbox_inches='tight', pad_inches=0.05)
# plt.show()
