import matplotlib.pyplot as plt

ir_values = []
peak_x = []
peak_y = []

with open("ir_filtered.txt", "r") as f:
    for line in f:
        try:
            line_str = line.strip()
            if not line_str:
                continue
                
            parts = line_str.split(",")
            if len(parts) < 2:
                continue

            ir = float(parts[0].split(":")[1].strip())
            peak = parts[1].split(":")[1].strip().lower() == "yes"

            ir_values.append(ir)

            if peak:
                # Use len(ir_values) - 1 so index matches 
                # even if some header/empty lines were skipped
                peak_x.append(len(ir_values) - 1)
                peak_y.append(ir)
                
        except (ValueError, IndexError):
            # Safely skips header rows, footers, or malformed data lines
            continue

# Draw signal
plt.plot(ir_values, color="blue", linewidth=1, label="IR")

# Draw peak markers
plt.scatter(
    peak_x,
    peak_y,
    color="green",
    s=50,
    marker="o",
    label="Peaks",
    zorder=5
)

plt.title("IR Signal with Peaks")
plt.xlabel("Sample")
plt.ylabel("IR")
plt.grid(True)
plt.legend()

plt.show()