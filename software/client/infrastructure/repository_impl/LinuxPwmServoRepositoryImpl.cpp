#include "LinuxPwmServoRepositoryImpl.h"
#include <QDebug>

bool LinuxPwmServoRepositoryImpl::openDoor() {
    qDebug() << "[LinuxPwmServoRepo] 【打桩/驱动模拟】修改 sysfs/duty_cycle 成功 -> 舵机旋转 90° [门已开]";
    return true;
}

bool LinuxPwmServoRepositoryImpl::closeDoor() {
    qDebug() << "[LinuxPwmServoRepo] 【打桩/驱动模拟】恢复 sysfs/duty_cycle 成功 -> 舵机归位 [门已关]";
    return true;
}