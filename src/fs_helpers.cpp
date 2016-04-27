#include <boost/filesystem/operations.hpp>
#include <iostream>
#include <algorithm>
#include "../include/fs_helpers.hpp"
#include "../include/debug.hpp"

namespace llscm {
    using namespace std;
    using namespace boost::filesystem;

    static string exec_path;
    static deque<string> search_path = {
            "/usr/local/lib",
            "/lib",
            "/usr/lib"
    };

    void initExecPath(char *argv0) {
        path full_path(initial_path<path>());
        full_path = canonical(system_complete(path(argv0)));
        exec_path = full_path.parent_path().generic_string();

        search_path.push_front(exec_path);

        D(cerr << "Exec path: " << exec_path << endl);
    }

    pair<string, bool> getLibraryPath(const string & name) {
        for(auto & p: search_path) {
            auto res = findFileInDirectory(name, p);
            if (res.second) {
                return res;
            }
        }

        return {"", false};
    }

    pair<string, bool> findFileInDirectory(const string & fname, const string & dir) {
        path file = fname;
        directory_iterator dir_it(dir);
        directory_iterator end;

        auto file_it = find_if(dir_it, end, [&file] (const directory_entry & e) {
            return e.path().filename() == file;
        });

        if (file_it == end) {
            return {"", false};
        }

        return {file_it->path().generic_string(), true};
    }
}
