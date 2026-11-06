import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

M = 100
lines = open("esp32_ham.txt", encoding="utf-8", errors="ignore").read().splitlines()
bas = lines.index("CSV_BASLA")
bit = lines.index("CSV_BITIS", bas)
cyc = np.array([int(x) for x in lines[bas+1:bit]])
esp_ns = cyc / 240e6 / M * 1e9

pi_ilk = np.loadtxt("pi_bloklar_ilk.csv") / M
pi_sonra = np.loadtxt("pi_bloklar_sonra.csv") / M

for ad, v, birim in (("ESP32-S3", esp_ns, "ns"),
                     ("Pi 4 bekleme sonrasi ilk", pi_ilk, "us"),
                     ("Pi 4 hemen ardindan", pi_sonra, "us")):
    print(f"{ad}: n={len(v)} min={v.min():.2f} maks={v.max():.2f} "
          f"ort={v.mean():.2f} sd={v.std():.3f} {birim} | "
          f"maks/min={v.max()/v.min():.2f}")

fig, ax = plt.subplots(1, 3, figsize=(15, 4))
ax[0].hist(esp_ns, bins=50, color="tab:blue")
ax[0].set_yscale("log")
ax[0].set_title("ESP32-S3 (240 MHz, register)")
ax[0].set_xlabel("periyot suresi (ns)"); ax[0].set_ylabel("blok sayisi (log)")
ax[1].hist(pi_ilk, bins=50, color="tab:red")
ax[1].set_title("Pi 4: bekleme sonrasi ilk calistirma")
ax[1].set_xlabel("periyot suresi (us)"); ax[1].set_ylabel("blok sayisi")
ax[2].hist(pi_sonra, bins=50, color="tab:green")
ax[2].set_title("Pi 4: hemen ardindan")
ax[2].set_xlabel("periyot suresi (us)"); ax[2].set_ylabel("blok sayisi")
plt.tight_layout()
plt.savefig("histogram.png", dpi=150)
print("histogram.png kaydedildi")