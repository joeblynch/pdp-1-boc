def byte_to_bits(byte):
    bits = ['○' if byte & (1 << bit) else ' ' for bit in range(7, -1, -1)]
    return ''.join(bits[:5]) + '·' + ''.join(bits[5:])

def read_file_bits(filename):
    i = 0
    with open(filename, 'rb') as f:
        byte = f.read(1)
        while byte:
            # print(f'{i // 3} {byte_to_bits(ord(byte))}')
            print(f'{i} {byte_to_bits(ord(byte))}')
            # print(byte_to_bits(ord(byte)))
            byte = f.read(1)
            i += 1

# Example usage
filename = 'title.bin'  # Replace with your file name
# filename = '../hc_binmaker/boc-olson.bin'  # Replace with your file name
read_file_bits(filename)