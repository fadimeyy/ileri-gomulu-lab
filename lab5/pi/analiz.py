import csv, statistics, sys

print("| Dosya | Örnek | Pi ort. hız (örnek/s) | ESP aralık ort. (ms) | ESP aralık std (ms) | min / maks (ms) | ESP hızı (Hz) |")
print("|---|---|---|---|---|---|---|")
for yol in sys.argv[1:]:
    with open(yol, newline="") as f:
        satirlar = list(csv.DictReader(f))
    t0 = int(satirlar[0]["esp_zaman_ms"])
    s = [r for r in satirlar if int(r["esp_zaman_ms"]) - t0 >= 3000]
    esp = [int(r["esp_zaman_ms"]) for r in s]
    pi = [float(r["pi_zaman_s"]) for r in s]
    d = [b - a for a, b in zip(esp, esp[1:])]
    pi_hiz = (len(s) - 1) / (pi[-1] - pi[0])
    ort = statistics.mean(d)
    print(f"| {yol} | {len(s)} | {pi_hiz:.1f} | {ort:.2f} | {statistics.pstdev(d):.2f} | {min(d)} / {max(d)} | {1000/ort:.1f} |")
