#ifndef LINUX_PWM_SERVO_REPOSITORY_IMPL_H
#define LINUX_PWM_SERVO_REPOSITORY_IMPL_H

#include "domain/repository/IDoorAccessRepository.h"

class LinuxPwmServoRepositoryImpl : public IDoorAccessRepository {
public:
    LinuxPwmServoRepositoryImpl() = default;
    ~LinuxPwmServoRepositoryImpl() override = default;

    bool openDoor() override;
    bool closeDoor() override;
};

#endif // LINUX_PWM_SERVO_REPOSITORY_IMPL_H