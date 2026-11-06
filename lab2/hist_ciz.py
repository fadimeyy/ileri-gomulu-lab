#!/usr/bin/env python3
"""Lab 2 Gorev 4: monitor kayitlarindan periyot histogramlarini cizer.

Kullanim:
    python hist_ciz.py kayit_wifi_kapali.txt kayit_wifi_acik.txt
    python hist_ciz.py kayit_wifi_kapali.txt            (tek kayit)
    python hist_ciz.py a.txt b.txt -o grafik.png

Kayit dosyasinda su iki tur satir aranir:
    hist,<kaynak>,<kutu>,<sayi>          (1 us genisliginde histogram satiri)
    ... GPTimer n= ... ort= ... ss= ... min= ... max= ...   (2 saniyelik ozet)
"""
import argparse
import math
import re
from collections import defaultdict
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402

ANSI = re.compile(r"\x1b\[[0-9;]*[A-Za-z]")
HIST = re.compile(r"hist,(GPTimer|esp_timer),([<>]?\d+),(\d+)")
STAT = re.compile(
    r"(GPTimer|esp_timer)\s+n=\s*(\d+)\s+ort=\s*([\d.]+)\s+ss=\s*([\d.]+)"
    r"\s+min=\s*([\d.]+)\s+max=\s*([\d.]+)"
)
KAYNAKLAR = ["GPTimer", "esp_timer"]
RENKLER = {"GPTimer": "#1f77b4", "esp_timer": "#d95f02"}


def kutu_konumu(etiket):
    """'<400' -> 399, '>600' -> 601, '500' -> 500 (grafikte x konumu)."""
    if etiket.startswith("<"):
        return int(etiket[1:]) - 1
    if etiket.startswith(">"):
        return int(etiket[1:]) + 1
    return int(etiket)


def oku(yol):
    hist = defaultdict(lambda: defaultdict(int))
    ozet = defaultdict(list)
    with open(yol, encoding="utf-8", errors="replace") as f:
        for satir in f:
            satir = ANSI.sub("", satir)
            m = HIST.search(satir)
            if m:
                hist[m.group(1)][m.group(2)] += int(m.group(3))
                continue
            m = STAT.search(satir)
            if m:
                ad, n, ort, ss, mn, mx = m.groups()
                ozet[ad].append((int(n), float(ort), float(ss), float(mn), float(mx)))
    return hist, ozet


def birlestir(kayitlar):
    """2 saniyelik raporlari tek istatistikte birlestirir (agirlikli ortalama, birlesik ss)."""
    n_top = sum(k[0] for k in kayitlar)
    if n_top == 0:
        return None
    ort = sum(k[0] * k[1] for k in kayitlar) / n_top
    var = sum(k[0] * (k[2] ** 2 + k[1] ** 2) for k in kayitlar) / n_top - ort ** 2
    return {
        "n": n_top,
        "ort": ort,
        "ss": math.sqrt(var) if var > 0 else 0.0,
        "min": min(k[3] for k in kayitlar),
        "max": max(k[4] for k in kayitlar),
    }


def baslik(yol):
    ad = Path(yol).stem.lower()
    if "kapali" in ad:
        return "Wi-Fi kapalı"
    if "acik" in ad:
        return "Wi-Fi açık"
    return Path(yol).stem


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("dosyalar", nargs="+")
    ap.add_argument("-o", "--cikti", default="histogram.png")
    args = ap.parse_args()

    veriler = [(d, *oku(d)) for d in args.dosyalar]
    fig, eksenler = plt.subplots(1, len(veriler), figsize=(6.5 * len(veriler), 4.6), squeeze=False)

    ozet_satirlari = []
    for ax, (dosya, hist, ozet) in zip(eksenler[0], veriler):
        konumlar = []
        for ad in KAYNAKLAR:
            kutular = hist.get(ad, {})
            if not kutular:
                continue
            xs = sorted(kutu_konumu(k) for k in kutular)
            konumlar += xs
        if not konumlar:
            ax.set_title(f"{baslik(dosya)} (hist satırı yok)")
            continue
        lo, hi = min(konumlar), max(konumlar)

        for i, ad in enumerate(KAYNAKLAR):
            kutular = hist.get(ad, {})
            if not kutular:
                continue
            xs = [kutu_konumu(k) for k in kutular]
            ys = [kutular[k] for k in kutular]
            st = birlestir(ozet.get(ad, []))
            etiket = ad if st is None else f"{ad} (ss={st['ss']:.3f} µs)"
            ax.bar([x + (i - 0.5) * 0.4 for x in xs], ys, width=0.4,
                   color=RENKLER[ad], label=etiket)
            if st:
                ozet_satirlari.append((baslik(dosya), ad, st))

        ax.set_yscale("log")
        ax.set_xlim(lo - 3, hi + 3)
        ax.set_xlabel("Çağrı aralığı (µs)")
        ax.set_ylabel("Örnek sayısı (log)")
        ax.set_title(baslik(dosya))
        ax.grid(True, which="both", alpha=0.3)
        ax.legend()

    fig.suptitle("Lab 2 Görev 4: 500 µs periyotlu zamanlayıcıların çağrı aralığı dağılımı")
    fig.tight_layout()
    fig.savefig(args.cikti, dpi=150)
    print(f"Grafik kaydedildi: {args.cikti}\n")

    print("| Kayıt | Kaynak | n | Ortalama (µs) | Std. sapma (µs) | En küçük / en büyük (µs) |")
    print("|---|---|---|---|---|---|")
    for kayit, ad, st in ozet_satirlari:
        print(f"| {kayit} | {ad} | {st['n']} | {st['ort']:.3f} | {st['ss']:.3f} | "
              f"{st['min']:.3f} / {st['max']:.3f} |")


if __name__ == "__main__":
    main()
