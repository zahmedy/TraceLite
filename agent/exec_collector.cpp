#include <iostream>
#include
#include <bpf/libbpf.h>

int main(int argc, char const *argv[])
{
    struct bpf_object *obj =
        bpf_object__open_file("bpf/exec.bpf.o", nullptr);

    if (!obj)
    {
        std::cerr << "failed to open object: "
                  << strerror(errno) << "\n";
        return 1;
    }

    std::cout << "eBPF object opened successfully!\n";

    int err = bpf_object__load(obj);

    if (err < 0)
    {
        std::cerr << "Failed to load eBPF: "
                  << strerror(-err) << "\n";

        bpf_object__close(obj);
        return 1;
    }

    std::cout << "eBPF load successfully!\n";

    struct bpf_program *prog =
        bpf_object__find_program_by_name(obj, "handle_execve");

    if (!prog)
    {
        std::cerr << "Cannot find handle_execve\n";
        bpf_object__close(obj);
        return 1;
    }

    struct bpf_link *link =
        bpf_program__attach_tracepoint(
            prog,
            "syscalls",
            "sys_enter_execve");

    if (!link)
    {
        std::cerr << "Failed to attach: "
                  << strerror(errno) << "\n";
        bpf_object__close(obj);
        return 1;
    }

    std::cout << "Tracepoint attached! Press Enter to stop.\n";

    std::cin.get();

    // clean up
    bpf_link__destroy(link);
    bpf_object__close(obj);

    /* code */
    return 0;
}
