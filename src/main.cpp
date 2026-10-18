#include "helpers/logger.h"
#include "memory/memory.h"
#include "memory/initarray.h"
#include "memory/abortrecovery.h"
#include "pvz2/PassDamageArmor/PassDamageArmor.h"
#include "pvz2/UmbrellaLeaf/UmbrellaLeaf.h"
#include "pvz2/QuestRewardsFix/QuestUnlockPlantIconFix.h"
#include "pvz2/LawnZoomFix/LawnZoomFix.h"
#include <jni.h>
#include <dobby.h>


jint JNI_OnLoad(JavaVM* vm, void*)
{
    LOGI("SCT JNI_OnLoad");

    if (!AbortRecovery::Install())
    {
        LOGE("JNI_OnLoad: AbortRecovery::Install failed - continuing without abort protection");
    }

    if (!Memory::Initialize())
    {
        LOGE("JNI_OnLoad: Memory::Initialize failed");
        return JNI_VERSION_1_6;
    }

    void* pvz2Base = reinterpret_cast<void*>(Memory::GetBase());
    if (!Memory::RunInitArray(pvz2Base))
    {
        LOGE("JNI_OnLoad: RunInitArray failed - aborting registration");
        return JNI_VERSION_1_6;
    }

    if (!InstallAll())
    {
        LOGE("InstallAll failed");
    }
    else
    {
        LOGI("InstallAll registered successfully");
    }
    if (!QuestUnlockPlantIconFix::Install())
    {
        LOGE("Install quest fix failed");
    }
    else
    {
        LOGI("Install registered successfully");
    }
    if (!UmbrellaLeaf::Install())
        LOGE("UmbrellaLeaf::Install failed");
    else
        LOGI("UmbrellaLeaf::Install registered successfully");

    if (!LawnZoomFix::Install())
        LOGE("LawnZoomFix::Install failed");
    else
        LOGI("LawnZoomFix::Install registered successfully");

    return JNI_VERSION_1_6;
}
