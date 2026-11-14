#include <assert.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <os/log.h>
#include <xpc/xpc.h>
#include <asl.h>
#include "os_log_s.h"

// DARLING_OS_LOG_STDERR: _os_log_impl in the real os_log.c, run in a child process per case (the switch is read
// once per process) with stdout and stderr redirected to files that the parent reads to EOF.

void *_NSConcreteGlobalBlock[32];
void dispatch_once(dispatch_once_t *token, void (^block)(void))
{
	if (!*token) {
		*token = 1;
		block();
	}
}
const struct { int unused; } OS_os_log_class;
void *_os_object_alloc(const void *cls, size_t size) { return calloc(1, size); }

aslmsg asl_new(unsigned type) { return malloc(1); }
int asl_set(aslmsg msg, const char *key, const char *value) { return 0; }
asl_object_t asl_open(const char *ident, const char *facility, unsigned opts) { return (asl_object_t)&OS_os_log_class; }
void asl_release(aslmsg msg) { free(msg); }
int asl_log(asl_object_t client, aslmsg msg, int level, const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	printf("asl:%d:", level);
	vprintf(format, ap);
	putchar('\n');
	va_end(ap);
	return 0;
}

static uint8_t buf[32];
static uint32_t size;
static struct os_log_s app = { .magic = OS_LOG_MAGIC, .subsystem = "com.example.app", .category = "ui" };
static struct os_log_s nameless = { .magic = OS_LOG_MAGIC, .subsystem = "", .category = "" };

// one %{public}s argument, the layout unpack_buffer.m decodes
static void build(const char *text)
{
	uint64_t pointer = (uint64_t)(uintptr_t)text;
	size = 0;
	buf[size++] = 0;
	buf[size++] = 1;
	buf[size++] = 2 << 4 | 2;
	buf[size++] = sizeof pointer;
	memcpy(&buf[size], &pointer, sizeof pointer);
	size += sizeof pointer;
}

static void *logger(void *arg)
{
	_os_log_impl(NULL, &app, OS_LOG_TYPE_DEFAULT, "user=%{public}s", buf, size);
	return NULL;
}

enum { ONCE, RACE, TYPED, NAMELESS, DISABLED, FORMAT_CHARS };

static void child(int what, const char *env, FILE *out, FILE *err)
{
	if (env)
		setenv("DARLING_OS_LOG_STDERR", env, 1);
	else
		unsetenv("DARLING_OS_LOG_STDERR");
	dup2(fileno(out), 1);
	dup2(fileno(err), 2);
	build("alice");
	switch (what) {
	case ONCE:
		logger(NULL);
		logger(NULL);
		break;
	case RACE: {
		pthread_t threads[8];
		for (int i = 0; i < 8; i++)
			pthread_create(&threads[i], NULL, logger, NULL);
		for (int i = 0; i < 8; i++)
			pthread_join(threads[i], NULL);
		break;
	}
	case TYPED:
		_os_log_impl(NULL, &app, OS_LOG_TYPE_ERROR, "user=%{public}s", buf, size);
		break;
	case NAMELESS:
		_os_log_impl(NULL, &nameless, OS_LOG_TYPE_DEFAULT, "user=%{public}s", buf, size);
		break;
	case FORMAT_CHARS:
		// printf directives inside an argument must come out literally, never be interpreted
		build("%s%n%x");
		logger(NULL);
		break;
	case DISABLED: {
		struct os_log_s disabled = { .magic = OS_LOG_DISABLED_MAGIC };
		_os_log_impl(NULL, &disabled, OS_LOG_TYPE_DEFAULT, "user=%{public}s", buf, size);
		break;
	}
	}
	fflush(stdout);
	_exit(0);
}

static char *slurp(FILE *file)
{
	static char data[4096];
	rewind(file);
	size_t n = fread(data, 1, sizeof data - 1, file);
	data[n] = 0;
	return data;
}

static int failures;

static void run(const char *name, int what, const char *env, const char *want_stderr, const char *want_stdout)
{
	FILE *out = tmpfile(), *err = tmpfile();
	pid_t pid = fork();
	if (pid == 0)
		child(what, env, out, err);
	int status;
	waitpid(pid, &status, 0);
	char got_out[4096], got_err[4096];
	snprintf(got_out, sizeof got_out, "%s", slurp(out));
	snprintf(got_err, sizeof got_err, "%s", slurp(err));
	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0 || strcmp(got_err, want_stderr) != 0 || strcmp(got_out, want_stdout) != 0) {
		fprintf(stderr, "FAIL %s: status %d\n  stderr got [%s] want [%s]\n  stdout got [%s] want [%s]\n",
			name, status, got_err, want_stderr, got_out, want_stdout);
		failures++;
	}
	fclose(out);
	fclose(err);
}

#define LINE "os_log[com.example.app/ui] type=0: user=alice\n"
#define ASL "asl:6:user=alice\n"

int main(void)
{
	run("unset", ONCE, NULL, "", ASL ASL);
	run("empty", ONCE, "", "", ASL ASL);
	run("zero", ONCE, "0", "", ASL ASL);
	run("other word", ONCE, "yes", "", ASL ASL);
	run("set", ONCE, "1", LINE LINE, ASL ASL);
	run("error type", TYPED, "1", "os_log[com.example.app/ui] type=16: user=alice\n", "asl:3:user=alice\n");
	run("default names", NAMELESS, "1", "os_log[(default)/(default)] type=0: user=alice\n", ASL);
	run("format characters in argument", FORMAT_CHARS, "1", "os_log[com.example.app/ui] type=0: user=%s%n%x\n", "asl:6:user=%s%n%x\n");
	run("disabled log", DISABLED, "1", "", "");
	run("threads set", RACE, "1", LINE LINE LINE LINE LINE LINE LINE LINE, ASL ASL ASL ASL ASL ASL ASL ASL);
	run("threads unset", RACE, NULL, "", ASL ASL ASL ASL ASL ASL ASL ASL);
	if (failures)
		return 1;
	puts("os-log-stderr: ok");
	return 0;
}
