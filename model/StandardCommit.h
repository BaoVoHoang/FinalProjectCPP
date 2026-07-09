#ifndef STANDARDCOMMIT_H
#define STANDARDCOMMIT_H
#include "Commit.h"
#include <map>

class StandardCommit : public Commit {
    private:
        // maps path -> content
        map<string, string> fileSnapshots;
    public:
        StandardCommit(
            string author,
            string message,
            string timestamp,
            string commitID
        );
        ~StandardCommit() override;
        void displayCommit() override;
        string getSummary() override;

        map<string, string> getFileSnapshots();
        void setFileSnapshots(map<string, string> fileSnapshots);
};

#endif // STANDARDCOMMIT_H
