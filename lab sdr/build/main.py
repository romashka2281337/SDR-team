import numpy as np
import matplotlib.pyplot as plt

filename = "tx_buff.pcm"

data = np.fromfile(filename, dtype=np.complex64)
real = np.real(data)
imag = np.imag(data)
count = np.arange(len(real))

plt.figure(figsize=(10,5))
plt.plot(count, real, color='blue', label='I (Real)')
plt.plot(count, imag, color='red', label='Q (Imag)')
plt.title("Треугольный I/Q сигнал (float complex)")
plt.xlabel("Сэмплы")
plt.ylabel("Амплитуда")
plt.legend()
plt.grid()
plt.show()