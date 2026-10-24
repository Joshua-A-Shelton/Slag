// Runs the validation tests; with --coverage, also lists every check in
// src/ that no test triggered and fails if there is one (M7).

#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace {

std::set<std::pair<std::string, int>> fired;

std::string fileName(const std::string& path)
{
    return std::filesystem::path(path).filename().string();
}

struct Site
{
    std::string file;
    int firstLine;
    int lastLine;
    std::string text;
};

// Check macro uses outside #define blocks, with the lines each spans.
std::vector<Site> findSites()
{
    std::vector<Site> sites;
    const char* macros[] = {"SLAG_VALIDATE(",        "SLAG_VALIDATE_LIVE(",      "SLAG_CHECK_LIVE(",
                            "SLAG_CHECK_LIVE_THIS(", "SLAG_VALIDATE_RECORDING(", "SLAG_VALIDATE_OUTSIDE_PASS(",
                            "SLAG_VALIDATE_IN_PASS("};
    for (const auto& entry : std::filesystem::recursive_directory_iterator(SLAG_SOURCE_DIR))
    {
        if (entry.path().extension() != ".cpp")
        {
            continue;
        }
        std::string path = entry.path().generic_string();
        if (path.find("/common/") == std::string::npos && path.find("/util/") == std::string::npos)
        {
            continue;
        }
        std::ifstream file(entry.path());
        std::vector<std::string> lines;
        for (std::string line; std::getline(file, line);)
        {
            lines.push_back(line);
        }
        bool inDefine = false;
        for (size_t i = 0; i < lines.size(); ++i)
        {
            const std::string& line = lines[i];
            bool starts = line.find("#define") != std::string::npos;
            if (starts || inDefine)
            {
                inDefine = !line.empty() && line.back() == '\\';
                continue;
            }
            for (const char* macro : macros)
            {
                size_t at = line.find(macro);
                if (at == std::string::npos)
                {
                    continue;
                }
                // The invocation ends where its parentheses balance.
                int depth = 0;
                size_t last = i;
                bool done = false;
                for (size_t j = i; j < lines.size() && !done; ++j)
                {
                    for (size_t k = j == i ? at : 0; k < lines[j].size(); ++k)
                    {
                        depth += lines[j][k] == '(' ? 1 : lines[j][k] == ')' ? -1 : 0;
                        if (depth == 0 && lines[j][k] == ')')
                        {
                            last = j;
                            done = true;
                            break;
                        }
                    }
                }
                sites.push_back({fileName(path), int(i + 1), int(last + 1), line});
                break;
            }
        }
    }
    return sites;
}

} // namespace

namespace slag {

void recordValidationSite(const char* file, int line)
{
    fired.insert({fileName(file), line});
}

} // namespace slag

int main(int argc, char** argv)
{
    bool coverage = false;
    std::vector<char*> arguments;
    for (int i = 0; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "--coverage") == 0)
        {
            coverage = true;
        }
        else
        {
            arguments.push_back(argv[i]);
        }
    }
    doctest::Context context(static_cast<int>(arguments.size()), arguments.data());
    int result = context.run();
    if (!coverage || context.shouldExit())
    {
        return result;
    }
    std::vector<Site> sites = findSites();
    size_t missing = 0;
    for (const Site& site : sites)
    {
        bool hit = false;
        for (int line = site.firstLine; line <= site.lastLine && !hit; ++line)
        {
            hit = fired.count({site.file, line}) > 0;
        }
        if (!hit)
        {
            ++missing;
            std::printf("never triggered: %s:%d: %s\n", site.file.c_str(), site.firstLine, site.text.c_str());
        }
    }
    std::printf("validation coverage: %zu of %zu checks triggered\n", sites.size() - missing, sites.size());
    return result != 0 ? result : missing == 0 ? 0 : 1;
}
