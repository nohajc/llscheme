#ifndef LLSCHEME_FS_HELPERS_HPP
#define LLSCHEME_FS_HELPERS_HPP

#include <string>
#include <utility>

namespace llscm {
    void initExecPath(char * argv0);
    void initCWDPath();
    std::pair<std::string, bool> getLibraryPath(const std::string & name);
    std::pair<std::string, bool> findFileInDirectory(const std::string & fname, const std::string & dir);
}

#endif //LLSCHEME_FS_HELPERS_HPP
