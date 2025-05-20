import numpy as np
import matplotlib.pyplot as plt

with open("newton_output.dat", "r") as f:
    header = f.readline().strip().split()
    WIDTH = int(header[0])
    HEIGHT = int(header[1])
    TEMPO = float(header[2])
    X_MIN = float(header[3])
    X_MAX = float(header[4])
    Y_MIN = float(header[5])
    Y_MAX = float(header[6])

    data = np.genfromtxt(f, dtype=int)

assert data.shape == (HEIGHT, WIDTH), "Dimensões incorretas!"

plt.figure(figsize=(8, 8))
plt.imshow(data, cmap="twilight_shifted", extent=[X_MIN, X_MAX, Y_MIN, Y_MAX])
plt.title(f"Fractal de Newton - {WIDTH}x{HEIGHT} - Tempo: {TEMPO:.2f}s")
plt.axis("off")
plt.tight_layout()
plt.savefig("newton_fractal.png", dpi=150)
plt.show()
