#!/bin/sh

# Usage: ./build.sh bpf_file.c (default is sched_ext.bpf.c)

# Set the default file
BPF_FILE=${1:-rr.bpf.c}

# Create the vmlinux header with all the eBPF Linux functions
# if it doesn'r exist
if [ ! -f vmlinux.h ]; then
    echo "Creating vmlinux.h"
    bpftool btf dump file /sys/kernel/btf/vmlinux format c > vmlinux.h
fi

# If you want to run this on your own computer, make sure
# you're pointing clang to the right path for your scx include folder
# Otherwise, the scheduler is gonna get flooded with type errors

# Compile the scheduler
clang -target bpf -g -O2 \
    -Wno-visibility \
    -I. \
    -I/home/jesse/scx/scheds/include \
    -c $BPF_FILE -o ${BPF_FILE}.o

