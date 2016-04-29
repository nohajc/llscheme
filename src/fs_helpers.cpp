#include <boost/filesystem/operations.hpp>
#include <iostream>
#include <algorithm>
#include "../include/fs_helpers.hpp"
#include "../include/debug.hpp"

namespace llscm {
    using namespace std;
    using namespace boost::filesystem;

    static deque<string> & getSearchPath() {
        static deque<string> search_path = {
                "/usr/local/lib",
                "/lib",
                "/usr/lib"
        };
        return search_path;
    }

    void initExecPath(char *argv0) {
        path full_path(initial_path<path>());
        full_path = canonical(system_complete(path(argv0)));
        string exec_path = full_path.parent_path().generic_string();
        D(cerr << "Exec path: " << exec_path << endl);

        getSearchPath().push_front(exec_path);
    }

    void initCWDPath() {
        path full_path(current_path());
        string cwd_path = full_path.generic_string();
        D(cerr << "Current working dir: " << cwd_path << endl);

        getSearchPath().push_front(cwd_path);
    }

    pair<string, bool> getLibraryPath(const string & name) {
        for(auto & p: getSearchPath()) {
            auto res = findFileInDirectory(name, p);
            if (res.second) {
                return res;
            }
        }

        return {"", false};
    }

    pair<string, bool> findFileInDirectory(const string & fname, const string & dir) {
        path file = fname;
        // We must handle directory in fname
        path dir_path = dir / file.parent_path();
        if (!exists(dir_path)) {
            return {"", false};
        }

        directory_iterator dir_it(dir_path);
        file = file.filename(); // basename
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
