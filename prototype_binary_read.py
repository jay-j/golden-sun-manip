#!/usr/bin/env python3

# the log files are pretty highly compressible
# compress: $ tar -czvf archive.tar.gz timestamp-*
# uncompress: 

file1 = "1658470907-action.log"
file2 = "1658470907-state.log"

# size of an action in bytes
# not counting the newline (+1 byte)
SIZE_ACTION = 5
SIZE_STATE = 1553

def file_get_size(fn):
    with open(fn, "rb") as fd:
        byte = fd.read(1)
        num_bytes = 0
        lines = 0
        while byte:
            if byte == '\n':
                lines += 1
            num_bytes += 1
            byte = fd.read(1)
            
        print(f"Read {num_bytes} bytes from {fn}")
        print(f"Found {lines} endlines")
            

if __name__ == "__main__":
    file_get_size(file1)
    file_get_size(file2)

# TODO read in batches of bytes to build arrays of action, state pairs