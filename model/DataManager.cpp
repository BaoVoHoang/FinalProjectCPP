#include "DataManager.h"
#include <fstream>
#include <iostream>

bool DataManager::saveData(const Repository repo, const string fileName) {

    ofstream outFile;
    outFile.open(fileName);

    if (!outFile) {
        cout << "Save file could not be opened" << endl;
        return false;
    }

    // basic save for now
    outFile << "RepositoryName:" << repo.getRepositoryName() << endl;
    outFile << "RepositoryPath:" << repo.getRepPath() << endl;

    //  save files vector
    //  save commits vector
    //  save commit snapshots

    outFile.close();

    return true;
}

bool DataManager::loadData(Repository repo, const string fileName) {

    ifstream inFile;
    inFile.open(fileName);

    if (!inFile) {
        cout << "Load file could not be opened" << endl;
        return false;
    }

    // read repository name
    //  read repository path
    //  rebuild files and commits

    inFile.close();

    return true;
}