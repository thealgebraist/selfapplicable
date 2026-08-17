#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT INT TERM

g++ -std=c++23 -Wall -Wextra -pedantic "$root/c_subset_compiler.cpp" -o "$tmp/c_subset_compiler"

expect_status() {
  source=$1
  expected=$2
  stem=$(basename "$source" .c)
  timeout 5 "$tmp/c_subset_compiler" "$root/$source" > "$tmp/$stem.s"
  as --64 "$tmp/$stem.s" -o "$tmp/$stem.o"
  ld "$tmp/$stem.o" -o "$tmp/$stem"
  set +e
  timeout 5 "$tmp/$stem"
  actual=$?
  set -e
  if test "$actual" -eq 124; then
    echo "FAIL: $source: target exceeded 5 second timeout" >&2
    exit 1
  fi
  test "$actual" -eq "$expected" || {
    echo "FAIL: $source: expected exit $expected, got $actual" >&2
    exit 1
  }
}

expect_reject() {
  source=$1
  if timeout 5 "$tmp/c_subset_compiler" "$root/$source" > "$tmp/reject.s" 2>/dev/null; then
    echo "FAIL: $source was accepted" >&2
    exit 1
  fi
}

expect_status fixtures/echo.c 0
expect_status fixtures/function_call.c 7
expect_status fixtures/function_pointer.c 6
expect_status fixtures/function_pointer_address.c 6
expect_status fixtures/function_pointer_explicit_deref.c 6
expect_status fixtures/function_pointer_global.c 6
expect_status fixtures/function_pointer_global_address.c 6
expect_status fixtures/function_pointer_global_explicit_deref.c 6
expect_status fixtures/function_pointer_global_binary.c 7
expect_status fixtures/function_pointer_global_binary_direct.c 7
expect_status fixtures/function_pointer_global_nullary.c 9
expect_status fixtures/function_pointer_global_nullary_direct.c 9
expect_status fixtures/function_pointer_nullary.c 9
expect_status fixtures/function_pointer_nullary_explicit_deref.c 9
expect_status fixtures/function_pointer_binary.c 7
expect_status fixtures/function_pointer_binary_explicit_deref.c 7
expect_status fixtures/function_pointer_alias.c 6
expect_status fixtures/function_pointer_binary_alias.c 7
expect_status fixtures/function_pointer_binary_alias_explicit_deref.c 7
expect_status fixtures/function_pointer_nullary_alias.c 9
expect_status fixtures/function_pointer_nested.c 9
expect_status fixtures/function_pointer_nested_unary.c 6
expect_status fixtures/function_pointer_nested_binary.c 7
expect_status fixtures/function_pointer_parameter.c 6
expect_status fixtures/function_pointer_binary_parameter.c 7
expect_status fixtures/function_pointer_typedef.c 6
expect_status fixtures/function_pointer_binary_typedef.c 7
expect_status fixtures/function_pointer_nullary_typedef.c 9
expect_status fixtures/function_pointer_typedef_parameter.c 6
expect_status fixtures/function_pointer_binary_typedef_parameter.c 7
expect_status fixtures/function_pointer_void.c 0
expect_status fixtures/function_pointer_void_binary.c 0
expect_status fixtures/function_pointer_void_typedef.c 0
expect_status fixtures/function_pointer_void_binary_typedef.c 0
expect_status fixtures/function_pointer_void_parameter.c 0
expect_status fixtures/function_pointer_void_binary_parameter.c 0
expect_status fixtures/function_pointer_equality.c 1
expect_status fixtures/function_pointer_struct_field.c 6
expect_status fixtures/function_pointer_struct_binary_field.c 7
expect_status fixtures/function_pointer_struct_void_field.c 0
expect_status fixtures/function_pointer_struct_nullary_field.c 9
expect_status fixtures/function_pointer_struct_typedef_field.c 6
expect_status fixtures/function_pointer_struct_arrow_field.c 6
expect_status fixtures/function_pointer_struct_arrow_binary_field.c 7
expect_status fixtures/function_pointer_struct_arrow_void_field.c 0
expect_status fixtures/function_pointer_struct_arrow_nullary_field.c 9
expect_status fixtures/function_pointer_struct_aggregate.c 6
expect_status fixtures/function_pointer_struct_binary_aggregate.c 7
expect_status fixtures/function_pointer_struct_nullary_aggregate.c 9
expect_status fixtures/function_pointer_struct_void_aggregate.c 0
expect_status fixtures/function_pointer_struct_typedef_aggregate.c 6
expect_status fixtures/function_pointer_struct_binary_typedef_aggregate.c 7
expect_status fixtures/function_pointer_struct_void_typedef_aggregate.c 0
expect_status fixtures/function_pointer_struct_void_binary_typedef_aggregate.c 0
expect_status fixtures/function_pointer_struct_typedef_arrow.c 6
expect_status fixtures/function_pointer_struct_binary_typedef_arrow.c 7
expect_status fixtures/function_pointer_struct_void_typedef_arrow.c 0
expect_status fixtures/function_pointer_struct_multiple_fields.c 6
expect_status fixtures/function_pointer_struct_mixed_fields.c 6
expect_status fixtures/function_pointer_struct_mixed_binary_fields.c 7
expect_status fixtures/function_pointer_struct_mixed_aggregate.c 6
expect_status fixtures/function_pointer_struct_mixed_binary_aggregate.c 7
expect_status fixtures/function_pointer_struct_nested_field.c 6
expect_status fixtures/switch_return.c 7
expect_status fixtures/switch_default_return.c 3
expect_status fixtures/switch_two_cases.c 7
expect_status fixtures/enum_switch.c 7
expect_status fixtures/enum_switch_implicit.c 8
expect_status fixtures/bitwise_return.c 2
expect_status fixtures/shift_return.c 4
expect_status fixtures/modulo_return.c 1
expect_reject fixtures/modulo_by_zero.c
expect_status fixtures/division_return.c 3
expect_reject fixtures/division_by_zero.c
expect_status fixtures/comparison_return.c 1
expect_status fixtures/logical_and_return.c 0
expect_status fixtures/logical_or_return.c 1
expect_status fixtures/ternary_true_return.c 7
expect_status fixtures/ternary_false_return.c 3
expect_status fixtures/logical_not_return.c 1
expect_status fixtures/negative_return.c 253
expect_status fixtures/addition_return.c 7
expect_status fixtures/subtraction_return.c 5
expect_status fixtures/multiplication_return.c 12
expect_status fixtures/char_sizeof_return.c 1
expect_status fixtures/char_pointer_sizeof_return.c 8
expect_status fixtures/character_literal_return.c 65
expect_status fixtures/character_escape_return.c 10
expect_status fixtures/character_tab_return.c 9
expect_status fixtures/character_quote_return.c 39
expect_status fixtures/character_backslash_return.c 92
expect_status fixtures/character_hex_return.c 65
expect_status fixtures/character_octal_return.c 65
expect_reject fixtures/character_bad_hex.c
expect_reject fixtures/character_bad_octal.c
expect_reject fixtures/character_multi_byte.c
expect_output() {
  source=$1
  expected=$2
  stem=$(basename "$source" .c)
  timeout 5 "$tmp/c_subset_compiler" "$root/$source" > "$tmp/$stem.s"
  as --64 "$tmp/$stem.s" -o "$tmp/$stem.o"
  ld "$tmp/$stem.o" -o "$tmp/$stem"
  set +e
  actual=$(timeout 5 "$tmp/$stem")
  status=$?
  set -e
  test "$status" -ne 124 || { echo "FAIL: $source: target exceeded 5 second timeout" >&2; exit 1; }
  test "$actual" = "$expected" || { echo "FAIL: $source: output mismatch" >&2; exit 1; }
}

expect_stdin_output() {
  source=$1
  input=$2
  expected=$3
  stem=$(basename "$source" .c)
  timeout 5 "$tmp/c_subset_compiler" "$root/$source" > "$tmp/$stem.s"
  as --64 "$tmp/$stem.s" -o "$tmp/$stem.o"
  ld "$tmp/$stem.o" -o "$tmp/$stem"
  set +e
  actual=$(printf '%s' "$input" | timeout 5 "$tmp/$stem")
  status=$?
  set -e
  test "$status" -ne 124 || { echo "FAIL: $source: target exceeded 5 second timeout" >&2; exit 1; }
  test "$actual" = "$expected" || { echo "FAIL: $source: stdin output mismatch" >&2; exit 1; }
}

expect_stderr() {
  source=$1
  expected=$2
  stem=$(basename "$source" .c)
  timeout 5 "$tmp/c_subset_compiler" "$root/$source" > "$tmp/$stem.s"
  as --64 "$tmp/$stem.s" -o "$tmp/$stem.o"
  ld "$tmp/$stem.o" -o "$tmp/$stem"
  set +e
  actual=$(timeout 5 "$tmp/$stem" 2>&1 >/dev/null)
  status=$?
  set -e
  test "$status" -ne 124 || { echo "FAIL: $source: target exceeded 5 second timeout" >&2; exit 1; }
  test "$actual" = "$expected" || { echo "FAIL: $source: stderr mismatch" >&2; exit 1; }
}

expect_combined_output() {
  source=$1
  expected=$2
  stem=$(basename "$source" .c)
  timeout 5 "$tmp/c_subset_compiler" "$root/$source" > "$tmp/$stem.s"
  as --64 "$tmp/$stem.s" -o "$tmp/$stem.o"
  ld "$tmp/$stem.o" -o "$tmp/$stem"
  set +e
  actual=$(timeout 5 "$tmp/$stem" 2>&1)
  status=$?
  set -e
  test "$status" -ne 124 || { echo "FAIL: $source: target exceeded 5 second timeout" >&2; exit 1; }
  test "$actual" = "$expected" || { echo "FAIL: $source: combined output mismatch" >&2; exit 1; }
}

expect_output fixtures/loop_break.c XX
expect_output fixtures/loop_continue.c XXXX
expect_output fixtures/cat.c "cat payload"
expect_status fixtures/mkdir_existing.c 1
expect_status fixtures/rm_missing.c 1
expect_status fixtures/rmdir_root.c 1
expect_status fixtures/touch_devnull.c 0
expect_status fixtures/chdir_root.c 0
expect_status fixtures/symlink_root.c 1
expect_status fixtures/link_root.c 1
expect_status fixtures/readlink_exe.c 0
expect_status fixtures/rename_root.c 1
expect_status fixtures/chmod_devnull.c 1
expect_status fixtures/access_devnull.c 0
expect_status fixtures/truncate_devnull.c 1
expect_status fixtures/getrandom_small.c 0
expect_status fixtures/write_stderr.c 0
expect_stderr fixtures/write_stderr.c "warning"
expect_stderr fixtures/write_stderr_adjacent.c "A"
expect_stderr fixtures/write_stderr_three_adjacent.c "AB"
expect_stderr fixtures/write_stderr_four_adjacent.c "AB"
expect_stderr fixtures/write_stderr_five_adjacent.c "ABC"
expect_stderr fixtures/write_stderr_two_calls.c "AB"
expect_status fixtures/write_mixed_streams.c 0
expect_combined_output fixtures/write_mixed_streams.c OE
expect_combined_output fixtures/write_mixed_streams_three.c OEO
expect_combined_output fixtures/write_mixed_streams_spaced.c OE
expect_combined_output fixtures/write_mixed_adjacent.c OEX
expect_combined_output fixtures/write_mixed_three_adjacent.c OEXY
expect_combined_output fixtures/write_mixed_four_adjacent.c OEXYZ
expect_combined_output fixtures/write_mixed_five_adjacent.c OEXYZ[
expect_status fixtures/readstdin_empty.c 0
expect_status fixtures/sleep_zero.c 0
expect_status fixtures/nice_one.c 0
expect_status fixtures/setpriority_query.c 1
expect_status fixtures/getgroups_query.c 0
expect_status fixtures/sched_setscheduler_query.c 1
expect_status fixtures/sethostname_query.c 1
expect_status fixtures/setdomainname_query.c 1
expect_status fixtures/get_thread_area_query.c 1
expect_status fixtures/set_thread_area_query.c 1
expect_status fixtures/ustat_query.c 1
expect_status fixtures/afs_syscall_query.c 1
expect_status fixtures/nfsservctl_query.c 1
expect_status fixtures/vserver_query.c 1
expect_status fixtures/isatty_stdin.c 1
expect_status fixtures/sync.c 0
expect_status fixtures/fsync_payload.c 0
expect_status fixtures/umask.c 0
expect_status fixtures/fdatasync_payload.c 0
expect_status fixtures/fcntl_getfd.c 0
expect_status fixtures/setpgid_self.c 0
expect_status fixtures/yield.c 0
expect_status fixtures/getpid_live.c 0
expect_status fixtures/getppid_live.c 0
expect_status fixtures/setpriority_self.c 0
expect_status fixtures/isroot_nonroot.c 1
expect_status fixtures/gettid_live.c 0
expect_status fixtures/isgroup0_nonroot.c 1
expect_status fixtures/nice_zero.c 0
expect_output fixtures/writefd_stdout.c Q
expect_stdin_output fixtures/readfd_stdin.c ABC ABC
expect_status fixtures/poll_zero.c 0
expect_status fixtures/alarm_zero.c 0
expect_status fixtures/clock_monotonic.c 0
expect_status fixtures/gettimeofday.c 0
expect_status fixtures/times.c 0
expect_status fixtures/getrusage_self.c 0
expect_status fixtures/sysinfo.c 0
expect_status fixtures/uname.c 0
expect_status fixtures/getdomainname.c 0
expect_status fixtures/fstat_stdout.c 0
expect_status fixtures/stat_payload.c 0
expect_status fixtures/lstat_payload.c 0
expect_status fixtures/isegroup0_nonroot.c 1
expect_status fixtures/getgroups.c 0
expect_status fixtures/getresuid.c 0
expect_status fixtures/getresgid.c 0
expect_status fixtures/getrlimit_cpu.c 0
expect_status fixtures/getpriority_self.c 0
expect_status fixtures/getcpu.c 0
expect_status fixtures/sched_affinity_self.c 0
expect_status fixtures/eventfd_zero.c 0
expect_status fixtures/timerfd_monotonic.c 0
expect_status fixtures/inotify_zero.c 0
expect_status fixtures/pidfd_init.c 0
expect_status fixtures/memfd_create.c 0
expect_status fixtures/epoll_create1.c 0
expect_status fixtures/epoll_create.c 0
expect_status fixtures/getuid.c 0
expect_status fixtures/geteuid.c 0
expect_status fixtures/getgid.c 0
expect_status fixtures/getegid.c 0
expect_status fixtures/getpgid_self.c 0
expect_status fixtures/getsid_self.c 0
expect_status fixtures/sched_getscheduler_self.c 0
expect_status fixtures/sched_getparam_self.c 0
expect_status fixtures/sched_priority_max.c 0
expect_status fixtures/sched_priority_min.c 0
expect_status fixtures/sched_rr_interval_self.c 0
expect_status fixtures/set_tid_address.c 0
expect_status fixtures/prctl_get_name.c 0
expect_status fixtures/prctl_get_dumpable.c 0
expect_status fixtures/prctl_get_no_new_privs.c 0
expect_status fixtures/prctl_get_seccomp.c 0
expect_status fixtures/prctl_get_timerslack.c 0
expect_status fixtures/prctl_get_child_subreaper.c 0
expect_status fixtures/prctl_get_ambient_zero.c 0
expect_status fixtures/prctl_get_pdeathsig.c 0
expect_status fixtures/prctl_get_tid_address.c 0
expect_status fixtures/prctl_get_thp_disable.c 0
expect_status fixtures/prctl_get_mce_kill.c 0
expect_status fixtures/capget.c 0
expect_status fixtures/statx_tmp.c 0
expect_status fixtures/listxattr_tmp.c 0
expect_status fixtures/flistxattr_stdout.c 0
expect_status fixtures/getxattr_tmp.c 1
expect_status fixtures/fgetxattr_stdout.c 1
expect_status fixtures/openat2_tmp.c 0
expect_status fixtures/close_range_test.c 0
expect_status fixtures/membarrier_query.c 0
expect_status fixtures/get_mempolicy_query.c 0
expect_status fixtures/faccessat2_tmp.c 0
expect_status fixtures/syncfs_stdout.c 0
expect_status fixtures/io_uring_setup_query.c 0
expect_status fixtures/io_uring_enter_query.c 0
expect_status fixtures/statfs_tmp.c 0
expect_status fixtures/fstatfs_stdout.c 0
expect_status fixtures/getdents64_tmp.c 0
expect_status fixtures/copy_file_range_zero.c 1
expect_status fixtures/readahead_stdout.c 1
expect_status fixtures/futex_wake_probe.c 0
expect_status fixtures/futex_wait_probe.c 1
expect_status fixtures/timerfd_gettime_query.c 0
expect_status fixtures/sched_getattr_query.c 0
expect_status fixtures/get_robust_list_query.c 0
expect_status fixtures/pidfd_send_signal_probe.c 1
expect_status fixtures/splice_zero.c 0
expect_status fixtures/sync_file_range_stdout.c 1
expect_status fixtures/tee_zero.c 0
expect_status fixtures/vmsplice_zero.c 0
expect_status fixtures/memfd_get_seals_query.c 0
expect_status fixtures/fcntl_getpipe_sz_query.c 0
expect_status fixtures/ioctl_pipe_fionread_query.c 0
expect_status fixtures/ioctl_pipe_fionbio_query.c 0
expect_status fixtures/fcntl_getfd_stdout.c 0
expect_status fixtures/fcntl_getfl_stdout.c 0
expect_status fixtures/fcntl_getown_stdout.c 0
expect_status fixtures/fcntl_getsig_stdout.c 0
expect_status fixtures/epoll_pwait_query.c 0
expect_status fixtures/ppoll_empty_query.c 0
expect_status fixtures/select_empty_query.c 0
expect_status fixtures/pselect6_empty_query.c 0
expect_status fixtures/io_uring_register_query.c 0
expect_status fixtures/eventfd_read_query.c 0
expect_status fixtures/eventfd_write_query.c 0
expect_status fixtures/memfd_secret_query.c 0
expect_status fixtures/rseq_query.c 1
expect_status fixtures/futex_waitv_query.c 1
expect_status fixtures/futex_query.c 1
expect_status fixtures/futex_wake_query.c 0
expect_status fixtures/process_mrelease_query.c 1
expect_status fixtures/cachestat_query.c 1
expect_status fixtures/set_mempolicy_home_node_query.c 0
expect_status fixtures/map_shadow_stack_query.c 1
expect_status fixtures/fchmodat2_query.c 1
expect_status fixtures/statmount_query.c 1
expect_status fixtures/listmount_query.c 1
expect_status fixtures/lsm_get_self_attr_query.c 1
expect_status fixtures/mseal_query.c 0
expect_status fixtures/futex_requeue_query.c 1
expect_status fixtures/futex_trylock_pi_query.c 1
expect_status fixtures/futex_cmp_requeue_query.c 1
expect_status fixtures/futex_lock_pi_query.c 0
expect_status fixtures/futex_unlock_pi_query.c 1
expect_status fixtures/futex_wake_op_query.c 1
expect_status fixtures/futex_wait_requeue_pi_query.c 1
expect_status fixtures/futex_wait_bitset_query.c 1
expect_status fixtures/futex_wake_bitset_query.c 1
expect_status fixtures/timer_getoverrun_query.c 1
expect_status fixtures/futex_lock_pi2_query.c 0
expect_status fixtures/clock_settime_query.c 1
expect_status fixtures/settimeofday_query.c 1
expect_status fixtures/semget_query.c 1
expect_status fixtures/msgget_query.c 1
expect_status fixtures/shmget_query.c 1
expect_status fixtures/shmat_query.c 1
expect_status fixtures/shmdt_query.c 1
expect_status fixtures/shmctl_query.c 1
expect_status fixtures/semctl_query.c 1
expect_status fixtures/semop_query.c 1
expect_status fixtures/semtimedop_query.c 1
expect_status fixtures/msgsnd_query.c 1
expect_status fixtures/msgrcv_query.c 1
expect_status fixtures/msgctl_query.c 1
expect_status fixtures/mq_open_query.c 1
expect_status fixtures/mq_unlink_query.c 1
expect_status fixtures/mq_timedsend_query.c 1
expect_status fixtures/mq_timedreceive_query.c 1
expect_status fixtures/mq_notify_query.c 1
expect_status fixtures/mq_getsetattr_query.c 1
expect_status fixtures/lsm_list_modules_query.c 1
expect_status fixtures/lsm_set_self_attr_query.c 1
expect_status fixtures/open_tree_query.c 1
expect_status fixtures/fsopen_query.c 1
expect_status fixtures/fsconfig_query.c 1
expect_status fixtures/fsmount_query.c 1
expect_status fixtures/fspick_query.c 1
expect_status fixtures/move_mount_query.c 1
expect_status fixtures/mount_setattr_query.c 1
expect_status fixtures/process_madvise_query.c 1
expect_status fixtures/process_vm_readv_query.c 0
expect_status fixtures/getrandom_query.c 0
expect_status fixtures/epoll_pwait2_query.c 1
expect_status fixtures/getpid_query.c 0
expect_status fixtures/sched_yield_query.c 0
expect_status fixtures/nanosleep_query.c 1
expect_status fixtures/clock_nanosleep_query.c 1
expect_status fixtures/clock_gettime_query.c 0
expect_status fixtures/clock_getres_query.c 0
expect_status fixtures/getitimer_query.c 1
expect_status fixtures/setitimer_query.c 0
expect_status fixtures/timer_create_query.c 0
expect_status fixtures/timer_gettime_query.c 1
expect_status fixtures/timer_settime_query.c 1
expect_status fixtures/timer_delete_query.c 1
expect_status fixtures/alarm_query.c 0
expect_status fixtures/sched_getaffinity_query.c 0
expect_status fixtures/sched_setaffinity_query.c 1
expect_status fixtures/sched_getcpu_query.c 0
expect_status fixtures/getpriority_query.c 0
expect_status fixtures/getrlimit_query.c 0
expect_status fixtures/getrusage_query.c 0
expect_status fixtures/getxattr_query.c 1
expect_status fixtures/listxattr_query.c 1
expect_status fixtures/removexattr_query.c 1
expect_status fixtures/lgetxattr_query.c 1
expect_status fixtures/llistxattr_query.c 0
expect_status fixtures/fgetxattr_query.c 1
expect_status fixtures/flistxattr_query.c 0
expect_status fixtures/inotify_rm_watch_query.c 1
expect_status fixtures/fremovexattr_query.c 1
expect_status fixtures/setxattr_query.c 1
expect_status fixtures/lsetxattr_query.c 1
expect_status fixtures/fchown_query.c 0
expect_status fixtures/chown_query.c 1
expect_status fixtures/lchown_query.c 1
expect_status fixtures/fchmodat_query.c 1
expect_status fixtures/faccessat_query.c 1
expect_status fixtures/faccessat2_query.c 1
expect_status fixtures/openat2_query.c 1
expect_status fixtures/statx_query.c 0
expect_status fixtures/statfs_query.c 0
expect_status fixtures/fstatfs_query.c 0
expect_status fixtures/pselect6_query.c 0
expect_status fixtures/ppoll_query.c 0
expect_status fixtures/select_query.c 0
expect_status fixtures/poll_query.c 0
expect_status fixtures/adjtimex_query.c 1
expect_status fixtures/clock_adjtime_query.c 1
expect_status fixtures/ioctl_query.c 1
expect_status fixtures/fcntl_query.c 0
expect_status fixtures/fcntl_getfl_query.c 0
expect_status fixtures/fcntl_dupfd_query.c 0
expect_status fixtures/fcntl_setfd_query.c 0
expect_status fixtures/fcntl_getown_query.c 0
expect_status fixtures/fcntl_getsig_query.c 1
expect_status fixtures/fcntl_setown_query.c 0
expect_status fixtures/fcntl_setfl_query.c 0
expect_status fixtures/fcntl_getlease_query.c 0
expect_status fixtures/fcntl_getseal_query.c 1
expect_status fixtures/fcntl_setlease_query.c 1
expect_status fixtures/fcntl_setpipe_sz_query.c 0
expect_status fixtures/fcntl_dupfd_cloexec_query.c 0
expect_status fixtures/fcntl_add_seals_query.c 0
expect_status fixtures/fcntl_get_rw_hint_query.c 0
expect_status fixtures/fcntl_set_rw_hint_query.c 0
expect_status fixtures/fadvise_query.c 1
expect_status fixtures/syncfs_query.c 0
expect_status fixtures/mlock_query.c 0
expect_status fixtures/mlock2_query.c 0
expect_status fixtures/mlockall_query.c 0
expect_status fixtures/munlock_query.c 0
expect_status fixtures/munlockall_query.c 0
expect_status fixtures/ioprio_get_query.c 0
expect_status fixtures/madvise_query.c 0
expect_status fixtures/mprotect_query.c 0
expect_status fixtures/mremap_query.c 1
expect_status fixtures/mincore_query.c 0
expect_status fixtures/process_vm_writev_query.c 0
expect_status fixtures/clone3_query.c 1
expect_status fixtures/userfaultfd_query.c 1
expect_status fixtures/kcmp_query.c 1
expect_status fixtures/bpf_query.c 1
expect_status fixtures/seccomp_query.c 0
expect_status fixtures/fanotify_init_query.c 1
expect_status fixtures/name_to_handle_at_query.c 1
expect_status fixtures/sync_file_range_query.c 1
expect_status fixtures/finit_module_query.c 1
expect_status fixtures/delete_module_query.c 1
expect_status fixtures/init_module_query.c 1
expect_status fixtures/sysfs_query.c 0
expect_status fixtures/uselib_query.c 1
expect_status fixtures/lookup_dcookie_query.c 1
expect_status fixtures/setns_query.c 1
expect_status fixtures/open_by_handle_at_query.c 1
expect_status fixtures/io_setup_query.c 1
expect_status fixtures/perf_event_open_query.c 1
expect_status fixtures/io_destroy_query.c 1
expect_status fixtures/io_submit_query.c 1
expect_status fixtures/io_cancel_query.c 1
expect_status fixtures/migrate_pages_query.c 1
expect_status fixtures/move_pages_query.c 0
expect_status fixtures/mbind_query.c 0
expect_status fixtures/set_mempolicy_query.c 0
expect_status fixtures/set_tid_address_query.c 0
expect_status fixtures/recvmmsg_query.c 1
expect_status fixtures/sendmmsg_query.c 1
expect_status fixtures/socketpair_query.c 1
expect_status fixtures/accept4_query.c 1
expect_status fixtures/shutdown_query.c 1
expect_status fixtures/getsockopt_query.c 1
expect_status fixtures/setsockopt_query.c 1
expect_status fixtures/socket_query.c 1
expect_status fixtures/bind_query.c 1
expect_status fixtures/listen_query.c 1
expect_status fixtures/connect_query.c 1
expect_status fixtures/accept_query.c 1
expect_status fixtures/getsockname_query.c 1
expect_status fixtures/getpeername_query.c 1
expect_status fixtures/recvfrom_query.c 1
expect_status fixtures/sendto_query.c 1
expect_status fixtures/recvmsg_query.c 1
expect_status fixtures/sendmsg_query.c 1
expect_status fixtures/pkey_alloc_query.c 0
expect_status fixtures/pkey_free_query.c 1
expect_status fixtures/pkey_mprotect_query.c 0
expect_status fixtures/quotactl_fd_query.c 1
expect_status fixtures/quotactl_query.c 1
expect_status fixtures/waitid_query.c 1
expect_status fixtures/wait4_query.c 1
expect_status fixtures/capget_query.c 1
expect_status fixtures/umount2_query.c 1
expect_status fixtures/readahead_query.c 1
expect_status fixtures/close_range_query.c 1
expect_status fixtures/getdents64_query.c 1
expect_status fixtures/openat_query.c 1
expect_status fixtures/mkdirat_query.c 1
expect_status fixtures/unlinkat_query.c 1
expect_status fixtures/newfstatat_query.c 1
expect_status fixtures/fchownat_query.c 1
expect_status fixtures/get_kernel_syms_query.c 1
expect_status fixtures/eventfd2_query.c 0
expect_status fixtures/dup3_query.c 1
expect_status fixtures/timerfd_create_query.c 0
expect_status fixtures/signalfd_query.c 1
expect_status fixtures/mmap_query.c 1
expect_status fixtures/munmap_query.c 1
expect_status fixtures/memfd_create_query.c 0
expect_status fixtures/brk_query.c 0
expect_status fixtures/remap_file_pages_query.c 1
expect_status fixtures/fsync_query.c 1
expect_status fixtures/fdatasync_query.c 1
expect_status fixtures/fstatat_query.c 1
expect_status fixtures/mknodat_query.c 1
expect_status fixtures/utimensat_query.c 1
expect_status fixtures/futimesat_query.c 1
expect_status fixtures/preadv2_query.c 1
expect_status fixtures/pwritev2_query.c 1
expect_status fixtures/readv_query.c 1
expect_status fixtures/writev_query.c 1
expect_status fixtures/preadv_query.c 1
expect_status fixtures/pwritev_query.c 1
expect_status fixtures/readlinkat_query.c 1
expect_status fixtures/renameat2_query.c 1
expect_status fixtures/symlinkat_query.c 1
expect_status fixtures/renameat_query.c 1
expect_status fixtures/landlock_add_rule_query.c 1
expect_status fixtures/landlock_restrict_self_query.c 1
expect_status fixtures/keyctl_query.c 1
expect_status fixtures/sched_setattr_query.c 1
expect_status fixtures/sched_getparam_query.c 1
expect_status fixtures/sched_setparam_query.c 1
expect_status fixtures/sched_getscheduler_query.c 1
expect_status fixtures/sched_get_priority_max_query.c 0
expect_status fixtures/sched_get_priority_min_query.c 0
expect_status fixtures/sched_rr_get_interval_query.c 1
expect_status fixtures/personality_query.c 0
expect_status fixtures/prlimit64_query.c 1
expect_status fixtures/setfsuid_query.c 0
expect_status fixtures/setfsgid_query.c 0
expect_status fixtures/getpgid_query.c 1
expect_status fixtures/getsid_query.c 1
expect_status fixtures/getpgrp_query.c 0
expect_status fixtures/fanotify_mark_query.c 1
expect_status fixtures/unshare_query.c 0
expect_status fixtures/setresuid_query.c 0
expect_status fixtures/setresgid_query.c 0
expect_status fixtures/setreuid_query.c 0
expect_status fixtures/setregid_query.c 0
expect_status fixtures/getresgid_query.c 0
expect_status fixtures/getresuid_query.c 0
expect_status fixtures/setuid_query.c 1
expect_status fixtures/setgid_query.c 1
expect_status fixtures/getuid_query.c 0
expect_status fixtures/getgid_query.c 0
expect_status fixtures/geteuid_query.c 0
expect_status fixtures/getegid_query.c 0
expect_status fixtures/gettid_query.c 0
expect_status fixtures/getppid_query.c 0
expect_status fixtures/setpgid_query.c 1
expect_status fixtures/setsid_query.c 0
expect_status fixtures/umask_query.c 0
expect_status fixtures/getcwd_query.c 0
expect_status fixtures/chdir_query.c 0
expect_status fixtures/fchdir_query.c 1
expect_status fixtures/mkdir_query.c 1
expect_status fixtures/rmdir_query.c 1
expect_status fixtures/unlink_query.c 1
expect_status fixtures/readlink_query.c 1
expect_status fixtures/symlink_query.c 1
expect_status fixtures/link_query.c 1
expect_status fixtures/rename_query.c 1
expect_status fixtures/access_query.c 0
expect_status fixtures/stat_query.c 0
expect_status fixtures/lstat_query.c 0
expect_status fixtures/getdents_query.c 1
expect_status fixtures/open_query.c 0
expect_status fixtures/close_query.c 0
expect_status fixtures/dup_query.c 0
expect_status fixtures/dup2_query.c 0
expect_status fixtures/pipe_query.c 0
expect_status fixtures/pipe2_query.c 0
expect_status fixtures/eventfd_query.c 0
expect_status fixtures/timerfd_query.c 0
expect_status fixtures/epoll_create_query.c 0
expect_status fixtures/epoll_ctl_query.c 1
expect_status fixtures/epoll_wait_query.c 1
expect_status fixtures/inotify_init_query.c 0
expect_status fixtures/inotify_add_watch_query.c 1
expect_status fixtures/memfd_query.c 0
expect_status fixtures/ftruncate_query.c 1
expect_status fixtures/fallocate_query.c 1
expect_status fixtures/sendfile_query.c 1
expect_status fixtures/copy_file_range_query.c 1
expect_status fixtures/splice_query.c 0
expect_status fixtures/tee_query.c 0
expect_status fixtures/vmsplice_query.c 0
expect_status fixtures/pidfd_open_query.c 1
expect_status fixtures/pidfd_send_signal_query.c 1
expect_status fixtures/pidfd_getfd_query.c 1
expect_status fixtures/sigaltstack_query.c 0
expect_status fixtures/rt_sigpending_query.c 1
expect_status fixtures/rt_sigtimedwait_query.c 1
expect_status fixtures/rt_sigqueueinfo_query.c 1
expect_status fixtures/rt_tgsigqueueinfo_query.c 1
expect_status fixtures/set_robust_list_query.c 1
expect_status fixtures/rt_sigprocmask_query.c 0
expect_status fixtures/rt_sigaction_query.c 1
expect_status fixtures/rt_sigsuspend_query.c 1
expect_status fixtures/restart_syscall_query.c 1
expect_status fixtures/arch_prctl_query.c 1
expect_status fixtures/modify_ldt_query.c 0
expect_status fixtures/io_pgetevents_query.c 1
expect_status fixtures/kexec_load_query.c 1
expect_status fixtures/kexec_file_load_query.c 1
expect_status fixtures/syslog_query.c 1
expect_status fixtures/sysctl_query.c 1
expect_status fixtures/setrlimit_query.c 1
expect_status fixtures/timerfd_settime_query.c 0
expect_status fixtures/signalfd4_query.c 0
expect_status fixtures/pidfd_getfd_probe.c 1
expect_status fixtures/landlock_create_ruleset_query.c 0
expect_status fixtures/madvise_probe.c 0
expect_status fixtures/mprotect_probe.c 0
expect_status fixtures/mremap_probe.c 0
expect_status fixtures/mincore_probe.c 0
expect_stdin_output fixtures/readstdin_four.c ABCD ABCD
expect_stdin_output fixtures/readstdin_eof_after_chunk.c ABCD ABCD
expect_status fixtures/dup_stdout_stderr.c 0
expect_status fixtures/close_stdin.c 0
expect_status fixtures/pipe_create.c 0
expect_output fixtures/write_escape.c "A	B"
expect_output fixtures/loop_write_escape.c "x	x	"
expect_output fixtures/loop_write_hex.c "AA"
expect_output fixtures/loop_write_quote.c 'A"BA"B'
expect_output fixtures/loop_write_apostrophe.c "A'BA'B"
expect_output fixtures/loop_write_question.c "A?BA?B"
expect_output fixtures/loop_write_adjacent.c "ABAB"
expect_output fixtures/loop_write_adjacent_hex.c "ABAB"
expect_output fixtures/loop_write_adjacent_mixed.c "?A?A"
expect_output fixtures/loop_write_adjacent_octal.c "ABAB"
expect_output fixtures/loop_write_adjacent_three.c "ABCABC"
expect_output fixtures/loop_write_empty_fragments.c "AA"
expect_output fixtures/loop_write_adjacent_four.c "ABCDABCD"
expect_output fixtures/loop_write_adjacent_five.c "ABCDEABCDE"
expect_output fixtures/write_backslash.c "A\\B"
expect_output fixtures/write_quote.c 'A"B'
expect_output fixtures/write_two.c "AB"
expect_output fixtures/write_three.c "ABC"
expect_bytes() {
  source=$1
  expected=$2
  stem=$(basename "$source" .c)
  timeout 5 "$tmp/c_subset_compiler" "$root/$source" > "$tmp/$stem.s"
  as --64 "$tmp/$stem.s" -o "$tmp/$stem.o"
  ld "$tmp/$stem.o" -o "$tmp/$stem"
  set +e
  actual=$(timeout 5 "$tmp/$stem" | od -An -tx1 | tr -d ' \n')
  status=$?
  set -e
  test "$status" -ne 124 || { echo "FAIL: $source: target exceeded 5 second timeout" >&2; exit 1; }
  test "$actual" = "$expected" || { echo "FAIL: $source: byte mismatch" >&2; exit 1; }
}
expect_bytes fixtures/loop_write_empty_nul.c "0000"
expect_bytes fixtures/loop_write_nul_empty_trailing.c "0000"
expect_bytes fixtures/loop_write_binary_three.c "00ff00ff"
expect_bytes fixtures/loop_write_binary_three_nonempty.c "4100ff4100ff"
expect_bytes fixtures/loop_write_binary_trailing_empty.c "4100ff4100ff"
expect_bytes fixtures/loop_write_binary_four.c "00ff00ff"
expect_bytes fixtures/loop_write_binary_four_nonempty.c "414200ff414200ff"
expect_bytes fixtures/loop_write_binary_four_full.c "4100ff424100ff42"
expect_bytes fixtures/loop_write_binary_five_full.c "414200ff43414200ff43"
expect_bytes fixtures/loop_write_adjacent_control.c "07080708"
expect_bytes fixtures/loop_write_adjacent_carriage_return.c "0d410d41"
expect_bytes fixtures/write_adjacent_nul.c "410042"
expect_bytes fixtures/write_adjacent_high_byte.c "ff41"
expect_bytes fixtures/write_adjacent_octal.c "4142"
expect_bytes fixtures/write_adjacent_control.c "0708"
expect_bytes fixtures/write_adjacent_whitespace.c "0a09"
expect_bytes fixtures/write_adjacent_form_vertical.c "0c0b"
expect_bytes fixtures/write_adjacent_binary_mixed.c "00ff"
expect_output fixtures/write_adjacent_mixed.c "?A"
expect_output fixtures/write_adjacent_empty.c "A"
expect_output fixtures/write_adjacent_empty_trailing.c "A"
expect_output fixtures/write_adjacent_empty_both.c "A"
expect_bytes fixtures/write_adjacent_mixed_whitespace.c "0a41"
expect_bytes fixtures/write_adjacent_mixed_tab_hex.c "0941"
expect_bytes fixtures/write_adjacent_mixed_cr_hex.c "0d41"
expect_bytes fixtures/write_adjacent_mixed_form_hex.c "0c41"
expect_bytes fixtures/write_adjacent_mixed_vertical_hex.c "0b41"
expect_bytes fixtures/write_adjacent_mixed_apostrophe_hex.c "2741"
expect_bytes fixtures/write_adjacent_mixed_backslash_hex.c "5c41"
expect_bytes fixtures/write_adjacent_mixed_backslash_octal.c "5c41"
expect_bytes fixtures/write_binary_four.c "00ff"
expect_bytes fixtures/write_binary_three.c "00ff"
expect_bytes fixtures/write_binary_three_nonempty.c "4100ff"
expect_bytes fixtures/write_binary_four_nonempty.c "414200ff"
expect_bytes fixtures/write_binary_four_full.c "4100ff42"
expect_bytes fixtures/loop_write_adjacent_mixed_octal.c "41094109"
expect_bytes fixtures/loop_write_adjacent_binary_mixed.c "410041410041"
expect_bytes fixtures/loop_write_adjacent_form_vertical.c "0c0b0c0b"
expect_bytes fixtures/loop_write_high_byte.c "ffff"
expect_bytes fixtures/loop_write_octal.c "4141"
expect_bytes fixtures/loop_write_hex_single.c "045a045a"
expect_bytes fixtures/write_carriage_return.c "410d42"
expect_bytes fixtures/loop_write_carriage_return.c "410d42410d42"
expect_bytes fixtures/loop_write_control_escapes.c "07080c0b07080c0b"
expect_bytes fixtures/loop_write_nul.c "410042410042"
expect_bytes fixtures/loop_write_adjacent_nul.c "410042410042"
expect_bytes fixtures/loop_write_adjacent_high_byte.c "ff41ff41"
expect_bytes fixtures/loop_write_adjacent_newline.c "0a410a41"
expect_bytes fixtures/loop_write_adjacent_tab.c "09410941"
expect_bytes fixtures/write_control_escapes.c "07080c0b"
expect_bytes fixtures/write_nul.c "410042"
expect_bytes fixtures/write_octal_string.c "414142"
expect_bytes fixtures/write_hex_string.c "414243"
expect_bytes fixtures/write_apostrophe.c "412742"
expect_bytes fixtures/write_high_byte.c "ff"
expect_bytes fixtures/write_question_escape.c "413f42"
expect_bytes fixtures/write_hex_single.c "41045a"
expect_bytes fixtures/write_adjacent.c "4142"
expect_bytes fixtures/write_adjacent_three.c "414243"
expect_bytes fixtures/write_adjacent_four.c "41424344"
expect_bytes fixtures/write_adjacent_five.c "4142434445"
expect_status fixtures/enum_sizeof_return.c 4
expect_reject fixtures/enum_sizeof_undeclared.c
expect_reject fixtures/enum_switch_duplicate.c
expect_reject fixtures/enum_switch_duplicate_name.c
expect_reject fixtures/function_pointer_alias_bad_type.c
expect_status fixtures/function_add.c 7
expect_status fixtures/function_constant.c 9
expect_status fixtures/struct_field_return.c 7
expect_status fixtures/struct_arrow_return.c 8
expect_status fixtures/struct_next_return.c 12
expect_status fixtures/pointer_array_assign.c 16
expect_status fixtures/pointer_add_deref.c 17
expect_status fixtures/array_sizeof_return.c 12
expect_status fixtures/struct_sizeof_return.c 12
expect_status fixtures/node_offsetof_return.c 4
expect_status fixtures/null_guard.c 1
expect_status fixtures/recursive_base_case.c 0
expect_output fixtures/while_write.c "www"
expect_output fixtures/while_write_zero.c ""
expect_output fixtures/while_write_inclusive_zero.c "K"
expect_output fixtures/loop_write_zero.c ""
expect_output fixtures/loop_write_inclusive.c "LLL"
expect_bytes fixtures/loop_write_inclusive_adjacent.c "4100ff4100ff"
expect_output fixtures/loop_write_inclusive_braced.c "BB"
expect_bytes fixtures/loop_write_braced_five.c "414200ff43414200ff43"
expect_bytes fixtures/loop_write_braced_adjacent_binary.c "4100ff4100ff"
expect_bytes fixtures/loop_write_braced_three_adjacent_binary.c "4100ff4100ff"
expect_bytes fixtures/loop_write_braced_four_adjacent_binary.c "4100ff4100ff"
expect_bytes fixtures/loop_write_inclusive_braced_adjacent_binary.c "4100ff4100ff"
expect_bytes fixtures/loop_write_inclusive_braced_three_adjacent_binary.c "4100ff4100ff"
expect_bytes fixtures/loop_write_inclusive_braced_four_adjacent_binary.c "4100ff4100ff"
expect_bytes fixtures/loop_write_inclusive_braced_five_adjacent_binary.c "4100ff4100ff"
expect_output fixtures/loop_write_two_body_writes.c "ABAB"
expect_bytes fixtures/loop_write_three_body_writes.c "410042410042"
expect_output fixtures/loop_write_inclusive_two_body_writes.c "ABAB"
expect_bytes fixtures/loop_write_inclusive_three_body_writes.c "410042410042"
expect_bytes fixtures/loop_write_inclusive_four_body_writes.c "4142004341420043"
expect_bytes fixtures/loop_write_inclusive_five_body_writes.c "414200ff43414200ff43"
expect_bytes fixtures/loop_write_inclusive_three.c "4100ff4100ff"
expect_bytes fixtures/loop_write_inclusive_four.c "414200ff414200ff"
expect_bytes fixtures/loop_write_inclusive_five.c "414200ff43414200ff43"
expect_bytes fixtures/while_write_adjacent_binary.c "41004100"
expect_bytes fixtures/while_write_three.c "4100ff4100ff"
expect_bytes fixtures/while_write_four.c "414200ff414200ff"
expect_bytes fixtures/while_write_five.c "414200ff43414200ff43"
expect_bytes fixtures/while_write_braced.c "51005100"
expect_bytes fixtures/while_write_braced_five.c "414200ff43414200ff43"
expect_output fixtures/while_write_two_body_writes.c "ABAB"
expect_bytes fixtures/while_write_three_body_writes.c "410042410042"
expect_bytes fixtures/while_write_four_body_writes.c "4142004341420043"
expect_bytes fixtures/while_write_five_body_writes.c "414200ff43414200ff43"
expect_output fixtures/while_write_inclusive_two_body_writes.c "ABAB"
expect_bytes fixtures/while_write_inclusive_three_body_writes.c "410042410042"
expect_bytes fixtures/while_write_inclusive_four_body_writes.c "4142004341420043"
expect_bytes fixtures/while_write_inclusive_five_body_writes.c "414200ff43414200ff43"
expect_output fixtures/while_write_explicit_increment.c "RRR"
expect_output fixtures/while_write_inclusive.c "III"
expect_output fixtures/do_write.c "DD"
expect_output fixtures/do_write_unbraced.c "UU"
expect_output fixtures/do_write_unbraced_inclusive.c "UU"
expect_bytes fixtures/do_write_unbraced_adjacent_binary.c "4100ff4100ff"
expect_bytes fixtures/do_write_unbraced_three_adjacent_binary.c "4100ff4100ff"
expect_bytes fixtures/do_write_unbraced_four_adjacent_binary.c "4100ff4100ff"
expect_bytes fixtures/do_write_unbraced_five_adjacent_binary.c "4100ff4100ff"
expect_bytes fixtures/do_write_unbraced_inclusive_adjacent_binary.c "4100ff4100ff"
expect_bytes fixtures/do_write_unbraced_inclusive_three_adjacent_binary.c "4100ff4100ff"
expect_bytes fixtures/do_write_unbraced_inclusive_four_adjacent_binary.c "4100ff4100ff"
expect_bytes fixtures/do_write_unbraced_inclusive_five_adjacent_binary.c "4100ff4100ff"
expect_output fixtures/do_write_inclusive.c "II"
expect_output fixtures/do_write_inclusive_zero.c "K"
expect_bytes fixtures/do_write_inclusive_adjacent_binary.c "4100ff4100ff"
expect_bytes fixtures/do_write_inclusive_three.c "4100ff4100ff"
expect_bytes fixtures/do_write_inclusive_four.c "414200ff414200ff"
expect_bytes fixtures/do_write_inclusive_five.c "414200ff43414200ff43"
expect_bytes fixtures/do_write_inclusive_two_body_writes.c "41004100"
expect_bytes fixtures/do_write_inclusive_three_body_writes.c "410042410042"
expect_bytes fixtures/do_write_inclusive_four_body_writes.c "4142004341420043"
expect_bytes fixtures/do_write_inclusive_five_body_writes.c "414200ff43414200ff43"
expect_output fixtures/do_write_zero.c "Z"
expect_output fixtures/do_write_empty.c ""
expect_output fixtures/do_write_inclusive_empty.c ""
expect_bytes fixtures/do_write_adjacent_binary.c "4100ff4100ff"
expect_bytes fixtures/do_write_three.c "4100ff4100ff"
expect_bytes fixtures/do_write_four.c "414200ff414200ff"
expect_bytes fixtures/do_write_five.c "414200ff43414200ff43"
expect_bytes fixtures/do_write_two_body_writes.c "41004100"
expect_bytes fixtures/do_write_three_body_writes.c "410042410042"
expect_bytes fixtures/do_write_four_body_writes.c "4142004341420043"
expect_bytes fixtures/do_write_five_body_writes.c "414200ff43414200ff43"
expect_bytes fixtures/while_write_inclusive_three.c "4100ff4100ff"
expect_bytes fixtures/while_write_inclusive_four.c "414200ff414200ff"
expect_bytes fixtures/while_write_inclusive_five.c "414200ff43414200ff43"
expect_bytes fixtures/while_write_inclusive_adjacent_binary.c "4100ff4100ff"
expect_bytes fixtures/while_write_inclusive_unbraced_increment_binary.c "4100ff4100ff"
expect_bytes fixtures/while_write_inclusive_braced_adjacent_binary.c "4100ff4100ff"
expect_bytes fixtures/while_write_unbraced_increment_binary.c "4100ff4100ff"
expect_output fixtures/while_write_inclusive_braced.c "JJ"
expect_status fixtures/global_return.c 4

expect_reject fixtures/unknown_call.c
expect_reject fixtures/function_add_bad_arity.c
expect_reject fixtures/function_pointer_bad_arity.c
expect_reject fixtures/function_pointer_global_bad_init.c
expect_reject fixtures/function_pointer_explicit_deref_bad_arity.c
expect_reject fixtures/function_pointer_global_binary_bad_arity.c
expect_reject fixtures/function_pointer_binary_bad_arity.c
expect_reject fixtures/function_pointer_global_binary_direct_bad_arity.c
expect_reject fixtures/function_pointer_global_nullary_bad_arity.c
expect_reject fixtures/struct_value_cycle.c
expect_reject fixtures/duplicate_global.c
expect_reject fixtures/duplicate_pointer_global.c
expect_reject fixtures/array_index_bad.c
expect_reject fixtures/write_bad_hex_string.c
expect_reject fixtures/write_bad_octal_string.c
expect_reject fixtures/loop_write_bad_hex.c
expect_reject fixtures/loop_write_bad_octal.c
expect_reject fixtures/loop_write_inclusive_bad_length.c
expect_reject fixtures/while_write_unbounded.c
expect_reject fixtures/do_write_unbounded.c
expect_reject fixtures/do_write_nonzero_init.c
expect_reject fixtures/do_write_nonincrement.c
expect_reject fixtures/do_write_inclusive_nonincrement.c
expect_reject fixtures/do_write_bad_length.c
expect_reject fixtures/do_write_inclusive_bad_length.c
expect_reject fixtures/loop_write_unbounded.c
expect_reject fixtures/loop_write_nonzero_init.c
expect_reject fixtures/loop_write_nonincrement.c
expect_reject fixtures/while_write_three_body_bad_length.c
expect_reject fixtures/write_adjacent_bad_hex.c
expect_reject fixtures/write_adjacent_bad_hex_digits.c
expect_reject fixtures/write_adjacent_bad_octal_digits.c
expect_reject fixtures/write_adjacent_bad_octal.c
expect_reject fixtures/write_adjacent_bad_escape.c
expect_reject fixtures/loop_write_adjacent_bad_escape.c

echo "assembler regression: PASS"
