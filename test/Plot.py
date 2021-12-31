from matplotlib import pyplot as plt
import subprocess
import json
import time

Sketch = ''
Metric = ''
Memory = []

with open('config.json', 'r') as f:
    data = json.load(f)
    Sketch = data['Sketch']
    Metric = data['Metric']
    Memory = data['Memory']

for sketch_name in Sketch:
    for metric_name in Metric:
        results = []
        for size in Memory:
            time.sleep(0.5)
            cmd = './driver -n' + sketch_name + ' -m ' + metric_name + ' -s ' + size
            run = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, close_fds=True)
            for out in run.stdout.readlines():
                res = str(out, encoding='utf-8')
                res = res.split(',')[2]
                results.append(float(res))
        print(sketch_name, metric_name, results)
        plt.figure()
        plt.plot(Memory, results, '--*m')
        plt.xlabel('Memory')
        plt.ylabel(metric_name)
        plt.title(sketch_name)
        plt.savefig(sketch_name+'-'+metric_name+'.jpg')
