#ifndef IDOOR_ACCESS_REPOSITORY_H
#define IDOOR_ACCESS_REPOSITORY_H

class IDoorAccessRepository {
public:
    virtual ~IDoorAccessRepository() = default;
    virtual bool openDoor() = 0;
    virtual bool closeDoor() = 0;
};

#endif // IDOOR_ACCESS_REPOSITORY_H