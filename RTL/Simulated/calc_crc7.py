import numpy as np

def calc_crc7(data: np.ndarray):
    # crc = 0x7F
    crc = 0x00
    for bit in data:
        new_input = ((crc >> 6) & 0x1) ^ bit
        crc = ((crc << 1) & 0x7F) ^ (0x09 if new_input else 0)
        print(f"{crc:02X}")
    return crc

arr = [1]
arr += [0]*38
arr[1:7] = [0, 0, 1, 0, 0, 0]
arr[27:31] = [0, 0, 0, 1]
arr[31:39] = [1, 0, 1, 0, 1, 0, 1, 0]
arr = np.array(arr, dtype=np.uint8)
# arr = arr[::-1]

print(f"{calc_crc7(arr):02X}")