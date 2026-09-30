#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>

struct exec_key
{
	/* The path supplied to execve, not a canonical executable path. */
	char filename[256];
};

struct
{
	__uint(type, BPF_MAP_TYPE_HASH);
	__uint(max_entries, 1024);
	__type(key, struct exec_key);
	__type(value, __u64);
} hash_map SEC(".maps");

SEC("tracepoint/syscalls/sys_enter_execve")

int handle_execve(struct trace_event_raw_sys_enter *ctx)
{
	struct exec_key key = {};

	const char *filename = (const char *)ctx->args[0];

	long ret = bpf_probe_read_user_str(
		key.filename,
		sizeof(key.filename),
		filename);

	if (ret <= 1)
	{
		return 0;
	}

	if (ret == sizeof(key.filename))
	{
		/* A full buffer may be either an exact fit or a truncated path.
		 * Check the original terminator so different long paths cannot
		 * be aggregated under the same truncated key.
		 */
		char terminator;
		if (bpf_probe_read_user(&terminator, sizeof(terminator),
								filename + sizeof(key.filename) - 1) < 0 ||
			terminator != '\0')
		{
			return 0;
		}
	}

	__u64 *value = bpf_map_lookup_elem(&hash_map, &key);

	if (value)
	{
		__sync_fetch_and_add(value, 1);
	}
	else
	{
		__u64 new_val = 1;
		bpf_map_update_elem(&hash_map, &key, &new_val, BPF_ANY);
	}

	return 0;
}

char LICENSE[] SEC("license") = "GPL";
