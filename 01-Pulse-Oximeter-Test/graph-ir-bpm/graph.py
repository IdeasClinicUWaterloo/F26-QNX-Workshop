
import re
import numpy as np
import matplotlib.pyplot as plt

FS = 200
FILENAME = "ir_filtered.txt"

ir_values = []
peak_flags = []

bpm_values = []
spo2_values = []
measurement_times = []

with open(FILENAME, "r", encoding="utf-8") as file:
    for line in file:

        # Example: IR: -16, peak: no
        ir_match = re.search(
            r"IR:\s*(-?\d+),\s*peak:\s*(yes|no)",
            line,
            re.IGNORECASE
        )

        if ir_match:
            ir_values.append(int(ir_match.group(1)))
            peak_flags.append(ir_match.group(2).lower() == "yes")
            continue
        
        result_match = re.search(
            r"(?:hr|heartrate)\s*[:=]\s*(-?\d+),\s*spo2\s*[:=]\s*(-?\d+)",
            line,
            re.IGNORECASE
        )

        if result_match:
            bpm = int(result_match.group(1))
            spo2 = int(result_match.group(2))

            measurement_times.append(len(ir_values) / FS)

            bpm_values.append(bpm if bpm >= 0 else np.nan)
            spo2_values.append(spo2 if spo2 >= 0 else np.nan)

        result_match = re.search(
            r"heartrate:(-?\d+),\s*spo2:(-?\d+)",
            line,
            re.IGNORECASE
        )

        if result_match:
            bpm = int(result_match.group(1))
            spo2 = int(result_match.group(2))

            # Use the number of IR samples processed
            measurement_times.append(len(ir_values) / FS)

            bpm_values.append(bpm if bpm >= 0 else np.nan)
            spo2_values.append(spo2 if spo2 >= 0 else np.nan)


# Create time axis
ir_values = np.array(ir_values)
peak_flags = np.array(peak_flags, dtype=bool)
ir_times = np.arange(len(ir_values)) / FS

# Create three graphs
fig, axes = plt.subplots(
    3, 1,
    figsize=(13, 9),
    sharex=True
)

# Graph 1: Filtered IR and peaks
axes[0].plot(ir_times, ir_values, label="Filtered IR")
axes[0].scatter(
    ir_times[peak_flags],
    ir_values[peak_flags],
    color="red",
    s=30,
    label="Detected Peaks",
    zorder=3
)
axes[0].set_ylabel("Filtered IR")
axes[0].set_title("MAX30102 Measurement Analysis")
axes[0].legend()
axes[0].grid(True)

# Graph 2: Heart rate
axes[1].plot(
    measurement_times,
    bpm_values,
    marker="o",
    label="BPM"
)
axes[1].set_ylabel("Heart Rate (BPM)")
axes[1].legend()
axes[1].grid(True)

# Graph 3: SpO2
axes[2].plot(
    measurement_times,
    spo2_values,
    marker="o",
    label="SpO2"
)
axes[2].set_ylabel("SpO2 (%)")
axes[2].set_xlabel("Estimated Time (seconds)")
axes[2].set_ylim(0, 105)
axes[2].legend()
axes[2].grid(True)

plt.tight_layout()
plt.show()
