/* Template class used to calculate simple repository
 * statistics such as commits and tracked file count.
 */
#ifndef ANALYTICS_H
#define ANALYTICS_H



#include <string>
using namespace std;

//template
template <typename T>
class AnalyticsEngine {
public:
    int computeTotalCommits(const T commits) {

        // commits vector size
        return commits.size();
    }

    int computeTrackedFilesCount(const T files) {

        // files vector size
        return files.size();
    }


    //PASS GETCOMMIT!!!!
    string computeMostModifiedFiles(const T commits) {

       
    // commits vector will come from Repository::getCommits()

    // create map<string, int> fileCounter

    // loop commits

        // convert Commit pointer to StandardCommit pointer
        // because getFileSnapshots() is only in StandardCommit

        // get fileSnapshots

        // loop snapshots and count each file path

    // TODO:
    // find highest count

        return "Most modified files not calculated yet";
    }
};

#endif