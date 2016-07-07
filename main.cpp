#include <cstdio>
#include <cstdlib>

#include <string>
#include <iostream>

#include <sss/utlstring.hpp>
#include <sss/path.hpp>
#include <sss/util/PostionThrow.hpp>

#include "TCircleBuffer.hpp"
#include "ByteStreamEditor.hpp"

const char * rule_dir = "rule";
const char * rule_suffix = ".rule";

void help_msg()
{
    std::string app = sss::path::basename(sss::path::getbin());
    std::cout
        << app << " [-r] ( rule-name | /path/to/rule ) [target-file ... ]"
        << std::endl;
}

void ensule_rule_path(std::string& rule_path)
{
    if (sss::path::filereadable(rule_path)) {
        return;
    }
    else {
        if (sss::path::suffix(rule_path) != rule_suffix) {
            rule_path.append(rule_suffix);
            if (sss::path::filereadable(rule_path)) {
                return;
            }
        }
        SSS_POSTION_THROW(std::runtime_error,
                          rule_path << " not readable");
    }
}

int main (int argc, char *argv[])
{
    try {
        (void) argc;
        (void) argv;

        TCircleBuffer<char> cb(10);

        if (argc < 3) {
            help_msg();
            return EXIT_SUCCESS;
        }

        int arg_idx = 1;
        bool replace = false;
        if (sss::is_equal(argv[arg_idx], "-r")) {
            replace = true;
            arg_idx++;
        }
        std::string rule_path;

        // TODO ¿ÉÊ¡ÂÔruleºó×º
        if (sss::path::is_absolute(argv[arg_idx])) {
            rule_path = argv[arg_idx];
        }
        else {
            if (argv[arg_idx][0] == '.') {
                rule_path = sss::path::append_copy(sss::path::getcwd(), argv[arg_idx]);
            }
            else {
                rule_path = sss::path::append_copy(sss::path::dirname(sss::path::getbin()), rule_dir);
                sss::path::append(rule_path, argv[arg_idx]);
            }
        }
        arg_idx++;
        
        ensule_rule_path(rule_path);

        ByteStreamEditor b {rule_path};

        for (int i = arg_idx; i < argc; i++ ) {
            if (replace) {
                b.translate(argv[i], "", true);
            }
            else {
                b.translate(argv[i], std::string(argv[i]) + ".ts", false);
            }
        }

        return EXIT_SUCCESS;
    }
    catch (std::exception& e) {
        std::cout << e.what() << std::endl;
    }
    catch (...) {
        std::cout << "unknown exception" << std::endl;
    }
}
