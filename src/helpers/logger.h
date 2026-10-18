#pragma once
#include <android/log.h>
#include <unistd.h>
#include <sys/syscall.h>

#define SCT_TAG "ASCENTIES"

#define LOGI(fmt, ...) __android_log_print(ANDROID_LOG_INFO,  SCT_TAG, "[pid=%d tid=%d] " fmt, getpid(), (int)syscall(SYS_gettid), ##__VA_ARGS__)
#define LOGW(fmt, ...) __android_log_print(ANDROID_LOG_WARN,  SCT_TAG, "[pid=%d tid=%d] " fmt, getpid(), (int)syscall(SYS_gettid), ##__VA_ARGS__)
#define LOGE(fmt, ...) __android_log_print(ANDROID_LOG_ERROR, SCT_TAG, "[pid=%d tid=%d] " fmt, getpid(), (int)syscall(SYS_gettid), ##__VA_ARGS__)
#define LOGD(fmt, ...) __android_log_print(ANDROID_LOG_DEBUG, SCT_TAG, "[pid=%d tid=%d] " fmt, getpid(), (int)syscall(SYS_gettid), ##__VA_ARGS__)
