import numpy as np
import matplotlib.pyplot as mpl

def main():
    filename = "meas_0.txt"
    with open(filename, 'r') as file:
        lines = file.readlines()
        length = len(lines)
        I = np.zeros(256)
        Q = np.zeros(256)
        i = 0
        q = 0
        maxval = (2 ** 16) / 2
        for data in range(length-2*256, length):
            # if data < 256:
            if data % 2 == 0:
                I[i] = int(lines[data]) / maxval
                i += 1
            else:
                Q[q] = int(lines[data]) / maxval
                q += 1
        comp = np.multiply(I, 1j * Q)
        mpl.figure(1)
        mpl.plot(I)
        mpl.plot(Q)
        # mpl.xlim([0, ])
        mpl.figure(3)
        mpl.plot(abs(comp))
        mpl.figure(2)
        f = np.fft.fft(comp)
        f = np.fft.fftshift(f)
        mpl.plot(20 * np.log10(abs(f)))
        mpl.figure(4)
        mpl.plot(abs(f) ** 2)
        mpl.show()

main()