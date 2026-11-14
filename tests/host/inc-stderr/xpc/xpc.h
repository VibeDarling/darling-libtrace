#pragma once
// os_log.c takes dispatch_once from here. Its only block captures statics, so it is a global block and needs no runtime.
typedef long dispatch_once_t;
extern void *_NSConcreteGlobalBlock[32];
void dispatch_once(dispatch_once_t *token, void (^block)(void));
