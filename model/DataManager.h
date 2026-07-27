#ifndef DATAMANAGER_H
#define DATAMANAGER_H

#include <string>
#include "Repository.h"

using namespace std;

class DataManager {
public:

    bool saveData(const Repository& repo, const string fileName);

    bool loadData(Repository& repo, const string fileName);
};

#endif