#include <iostream>
//#include <nuclear-data-reader/ame_data.hpp>
#include <nuclear-data-reader/massTable.hpp>

using namespace std;

int main(int argc, char * argv[]) {
    cout << "Hello World" << endl;
    MassTable t(2020);
    t.setFilePaths();
    cout << "Reading from: " << t.AME_masstable << endl;
    if(!t.readAMEMassFile(t.AME_masstable)) {
        cerr << "Something went wrong!!" << endl;
        return 1;
    }
    if(!t.populateInternalMassTable()) {
        cerr << "Something went wrong!!" << endl;
        return 1;
    }
    if(!t.outputTableToJSON()) {
        cerr << "Something went wrong!!" << endl;
        return 1;
    }
    return 0;
}
