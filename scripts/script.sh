echo "REMOVE /sys/fs/bpf/tracelite"
sudo rm -rf /sys/fs/bpf/tracelite
echo "CREATE /sys/fs/bpf/tracelite/maps"
sudo mkdir -p /sys/fs/bpf/tracelite/maps
echo "COMPILE C CODE"
clang -O2 -g -target bpf -D__TARGET_ARCH_x86 -I. -c exec.bpf.c -o exec.bpf.o
echo "LOAD the compiled eBPF object"
echo "PIN the loaded programs, pin the maps"
echo "ATTACH programs based on their section metadata."
sudo bpftool prog loadall exec.bpf.o /sys/fs/bpf/tracelite/programs pinmaps /sys/fs/bpf/tracelite/maps autoattach
echo "DUMP mapped events in /sys/fs/bpf/tracelite/maps/hash_map"
sudo bpftool map dump pinned /sys/fs/bpf/tracelite/maps/hash_map
