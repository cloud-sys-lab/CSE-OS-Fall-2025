import sys
import numpy as np
import matplotlib.pyplot as plt

POS_V = 10.0   # RS-232 positive voltage
NEG_V = -10.0  # RS-232 negative voltage


def load_and_process(filename, rise_fraction=0.1):
    data = np.loadtxt(filename)

    levels = data[:, 0]
    times  = data[:, 1]

    volts = np.where(levels > 0.5, POS_V, NEG_V)

    order = np.argsort(times)
    times = times[order]
    volts = volts[order]

    
    dt = np.median(np.diff(times)) if len(times) > 1 else 1.0

 
    times_ext = np.concatenate(([times[0] - dt], times, [times[-1] + dt]))
    volts_ext = np.concatenate(([NEG_V], volts, [NEG_V]))

   
    t_new = []
    v_new = []

    for i in range(len(times_ext) - 1):
        t0 = times_ext[i]
        t1 = times_ext[i + 1]
        v0 = volts_ext[i]
        v1 = volts_ext[i + 1]

        t_new.append(t0)
        v_new.append(v0)

        if v0 != v1:
            edge_dt = rise_fraction * (t1 - t0)

           
            t_new.append(t1 - edge_dt)
            v_new.append(v0)

            t_new.append(t1)
            v_new.append(v1)

    
    t_new.append(times_ext[-1])
    v_new.append(volts_ext[-1])

    # convert time to ms 
    t_ms = np.array(t_new) / 1000.0
    v_smooth = np.array(v_new)

    return t_ms, v_smooth


def main():
    if len(sys.argv) < 2:
        print("Usage: python plot_rs232_multi.py file1.txt file2.txt ...")
        sys.exit(1)

    plt.figure(figsize=(10, 4))

    for i, fname in enumerate(sys.argv[1:]):
        t, v = load_and_process(fname)

        # line styles
        if i == 0:
            linestyle = "-"
        elif i == 1:
            linestyle = "--"   # SECOND FILE IS DASHED
        else:
            linestyle = "-"

        plt.plot(t, v, linestyle=linestyle, label=fname)

    plt.xlabel("Time (ms)")
    plt.ylabel("Voltage (V)")
    plt.title("RS-232 Waveform")
    plt.grid(True, linestyle=":", alpha=0.6)
    plt.legend()
    plt.tight_layout()
    plt.show()


if __name__ == "__main__":
    main()
