#ifndef IALARM_REPOSITORY_H
#define IALARM_REPOSITORY_H

class IAlarmRepository {
public:
    virtual ~IAlarmRepository() = default;
    virtual bool triggerAlarm(int durationMs) = 0;
    virtual bool stopAlarm() = 0;
};

#endif // IALARM_REPOSITORY_H