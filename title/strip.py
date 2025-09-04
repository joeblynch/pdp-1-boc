def copy_until_msb_set(src_file, dest_file):
    with open(src_file, 'rb') as source, open(dest_file, 'wb') as dest:
        while True:
            byte = source.read(1)
            if not byte:
                print("End of file reached without finding MSB set.")
                break
            if byte[0] & 0x80:  # Check if MSB is set
                print(f"MSB set byte encountered: {byte[0]:#04x}. Stopping copy.")
                break
            dest.write(byte)

# Example Usage:
source_file = 'output.bin'
destination_file = 'title.bin'
copy_until_msb_set(source_file, destination_file)
