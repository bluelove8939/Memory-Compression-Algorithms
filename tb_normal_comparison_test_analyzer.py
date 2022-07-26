import os
import numpy as np
import matplotlib.pyplot as plt


categories = []
results = {}


for model_name in os.listdir(os.path.join(os.curdir, 'extractions')):
    result_path = os.path.join(os.curdir, 'extractions', model_name, 'comparison_results.csv')
    with open(result_path, 'rt') as file:
        content = file.readlines()

    algo_indexes = list(map(lambda x: x.strip(), content[0].split(',')))[1:]
    avg_cratio = np.array([0] * len(algo_indexes), dtype=float)

    for line in content[1:]:
        lineparsed = line.split(',')
        avg_cratio += np.array(list(map(lambda x: float(x.strip()), lineparsed[1:])))

    avg_cratio /= (len(content) - 1)

    categories.append(model_name.split('_')[0])

    for algo_name in algo_indexes:
        if algo_name not in results.keys():
            results[algo_name] = []

    for key, val in zip(algo_indexes, avg_cratio):
        results[key].append(val)

width_max = 0.8
width = width_max / len(results.keys())

x_axis = np.arange(len(categories))
for idx, (key, val) in enumerate(results.items()):
    plt.bar(x_axis + ((idx - (len(results.keys()) / 2) + 0.5) * width), val, width=width, label=key)
plt.xticks(x_axis, categories, rotation=0, ha='center')
plt.ylim([0.9, 4])

plt.legend()
plt.tight_layout()
plt.show()