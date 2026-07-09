#include "StandardCommit.h"

#include <iostream>

StandardCommit::~StandardCommit() = default;

void StandardCommit::displayCommit() {
    cout << "Author: " << this->getAuthor() << endl;
    cout << "Message: " << this->getMessage() << endl;
    cout << "Timestamp: " << this->getTimestamp() << endl;
    cout << "Commit ID: " << this->getCommitID() << endl;
    cout << "Files being commited (path):" << endl;
    for (auto &[fst, snd] : fileSnapshots) {
        cout << fst;
    }
}

// TODO: Implement getSummary
string StandardCommit::getSummary() {return "";}
