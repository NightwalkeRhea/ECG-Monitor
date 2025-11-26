import numpy as np
from scipy.signal import lfilter
from fir_coef import coefs, Fs  # uses your bandpass taps and sampling rate

# derivative kernel from Pan–Tompkins, scaled to Fs (original assumes 200 Hz)
deriv_kernel = (Fs / 8.0) * np.array([1, 2, 0, -2, -1])

def pan_tompkins_ref(ecg, fs=Fs, bandpass_taps=coefs, mwi_ms=150):
    # 1) Bandpass (your FIR)
    band = lfilter(bandpass_taps, 1.0, ecg)

    # 2) Derivative
    diff = lfilter(deriv_kernel, 1.0, band)

    # 3) Squaring
    squared = diff ** 2

    # 4) Moving-window integration (typical 150 ms)
    mwi_len = max(1, int(round(mwi_ms * fs / 1000.0)))
    mwi_kernel = np.ones(mwi_len) / mwi_len
    mwi = np.convolve(squared, mwi_kernel, mode="same")

    # 5) Simple adaptive threshold & peak picking
    thresh = 0.5 * (np.mean(mwi) + np.max(mwi))
    refractory = int(0.200 * fs)  # 200 ms lockout
    peaks = []
    i = 0
    while i < len(mwi):
        if mwi[i] > thresh:
            # local-maximum search in a small neighborhood
            win = mwi[i : min(i + refractory, len(mwi))]
            local_peak = i + np.argmax(win)
            peaks.append(local_peak)
            i = local_peak + refractory  # skip refractory window
        else:
            i += 1

    return {
        "band": band,
        "diff": diff,
        "squared": squared,
        "mwi": mwi,
        "r_peaks_idx": np.array(peaks, dtype=int),
        "threshold": thresh,
    }

if __name__ == "__main__":
    # Synthetic ECG-like test (1 Hz heartbeat + harmonics + noise)
    duration_s = 10
    t = np.arange(0, duration_s, 1.0 / Fs)
    ecg = 0.9 * np.sin(2 * np.pi * 1.0 * t) \
        + 0.15 * np.sin(2 * np.pi * 2.0 * t) \
        + 0.05 * np.sin(2 * np.pi * 3.0 * t) \
        + 0.02 * np.random.randn(len(t))

    res = pan_tompkins_ref(ecg)
    print("Detected R-peak sample indices:", res["r_peaks_idx"])
    print("Threshold used:", res["threshold"])
    # You can also save res["band"], res["diff"], res["squared"], res["mwi"] for FPGA cross-checks.
