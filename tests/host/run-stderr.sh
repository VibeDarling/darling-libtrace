#!/bin/sh
# Builds libsystem_trace/os_log.c with its decoder against stub headers and runs os-log-stderr.c. Needs clang; no Darling build.
set -eu
here=$(cd "$(dirname "$0")" && pwd)
root="$here/../.."
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
# a quoted include finds the real os_log_s.h next to the source first, so compile a copy beside the stub one
cp "$root/libsystem_trace/unpack_buffer.m" "$here/inc/os_log_s.h" "$tmp/"
case $(uname -s) in
Darwin) objc_runtime="" ;;
*) objc_runtime="-fobjc-runtime=gnustep-1.9" ;;
esac
# shellcheck disable=SC2086
clang -w -fsigned-char -x objective-c $objc_runtime -DOS_LOG_PRIVACY=0 -I "$here/inc" -I "$root/libsystem_trace" -I "$root/libsbuf" \
	-include "$tmp/os_log_s.h" -c "$tmp/unpack_buffer.m" -o "$tmp/unpack.o"
clang -w "-D__FBSDID(x)=" -I "$here/inc" -include "$here/inc/os/base.h" -include stdlib.h -include string.h -include ctype.h \
	-c "$root/libsbuf/libsbuf.c" -o "$tmp/sbuf.o"
clang -w -c "$here/stubs.c" -o "$tmp/stubs.o"
clang -w -fblocks -include stdlib.h -include string.h -I "$here/inc-stderr" -I "$here/inc" -I "$root/libsystem_trace" -c "$root/libsystem_trace/os_log.c" -o "$tmp/os_log.o"
clang -w -fblocks -I "$here/inc-stderr" -I "$root/libsystem_trace" -c "$here/os-log-stderr.c" -o "$tmp/test.o"
clang "$tmp/os_log.o" "$tmp/unpack.o" "$tmp/sbuf.o" "$tmp/stubs.o" "$tmp/test.o" -o "$tmp/test" -lpthread
ulimit -c 0
"$tmp/test"
