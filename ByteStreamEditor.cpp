#include <iostream>
#include <fstream>
#include <stdexcept>
#include <sstream>
#include <cctype>

#include <sss/spliter.hpp>
#include <sss/util/Parser.hpp>
#include <sss/util/PostionThrow.hpp>

#include "ByteStreamEditor.hpp"

#ifndef VALUE_MSG
#define VALUE_MSG(a) (#a) << " = `" << a << "`"
#endif

namespace  {
    typedef std::string::const_iterator Iter_t;
    typedef sss::util::Parser<Iter_t> parser_t;
    typedef parser_t::Rewinder rewinder_t;
    bool skip_space(Iter_t& it_beg, Iter_t it_end)
    {
        while (it_beg != it_end && std::isspace(*it_beg)) {
            it_beg++;
        }
        return true;
    }
    bool parse_es_hex(Iter_t& it_beg, Iter_t it_end, char& content) {
        // std::cout << __func__ << ":" << __LINE__ << " " << VALUE_MSG(*it_beg) << std::endl;
        char buf[2];
        bool is_ok =
            parser_t::parseChar(it_beg, it_end, 'x') &&
            parser_t::parseHexChar(it_beg, it_end, buf[0]) &&
            parser_t::parseHexChar(it_beg, it_end, buf[1]);
        if (is_ok) {
            content = parser_t::hexchar2number(buf[0]) << 4 | (parser_t::hexchar2number(buf[1]));
            // std::cout << __func__ << ":" << __LINE__ << ":" << buf[0] << buf[1] << "=" << int(content) << std::endl;
        }
        return is_ok;
    }
    bool parse_es_oct(Iter_t& it_beg, Iter_t it_end, char& content) {
        // std::cout << __func__ << ":" << __LINE__ << " " << VALUE_MSG(*it_beg) << std::endl;
        char buf[3];
        bool is_ok =
            parser_t::parseRangeChar(it_beg, it_end, '0', '7', buf[0]) &&
            parser_t::parseRangeChar(it_beg, it_end, '0', '7', buf[1]);
        if (is_ok) {
            content = (buf[0] - '0') << 3 | (buf[1] - '0');
        }
        if (is_ok && buf[0] <= '3') {
            if (parser_t::parseRangeChar(it_beg, it_end, '0', '7', buf[2])) {
                content *= 8;
                content += buf[2] - '0';
            }
        }
        return is_ok;
    }
    bool parse_es_reserved(Iter_t& it_beg, Iter_t it_end, char& content) {
        // std::cout << __func__ << ":" << __LINE__ << " " << VALUE_MSG(*it_beg) << std::endl;
        bool is_ok = parser_t::parseSetChar(it_beg, it_end, "0\\abfnrtv'\"", content);
        if (is_ok) {
            switch (content) {
            case '\\':
            case '"':
                break;

            case '0':
                content = '\0';
                break;

            case 'a':
                content = '\a';
                break;

            case 'b':
                content = '\b';
                break;

            case 'f':
                content = '\f';
                break;

            case 'n':
                content = '\n';
                break;

            case 'r':
                content = '\r';
                break;

            case 't':
                content = '\t';
                break;

            case 'v':
                content = '\v';
                break;
            }
            // std::cout << __func__ << " " << VALUE_MSG(content) << std::endl;
        }
        return is_ok;
    }

    bool parse_escape(Iter_t& it_beg, Iter_t it_end, char& content)
    {
        rewinder_t r(it_beg);

        if (parser_t::parseChar(it_beg, it_end, '\\')) {
            // std::cout << __func__ << ":" << __LINE__ << " " << VALUE_MSG(*it_beg) << std::endl;
            rewinder_t r_in(it_beg);
            if (r_in.commit(::parse_es_hex(it_beg, it_end, content)) ||
                r_in.commit(::parse_es_oct(it_beg, it_end, content)) ||
                r_in.commit(::parse_es_reserved(it_beg, it_end, content))) {
                r.commit(true);
            }
        }
        return r.is_commited();
    }

    bool parse_dq_str(Iter_t& it_beg, Iter_t it_end, std::string& content)
    {
        content.resize(0);
        bool is_ok = parser_t::parseChar(it_beg, it_end, '"');
        if (is_ok) {
            while (it_beg != it_end && *it_beg != '"') {
                // this->c_str_escapseq_p =
                //     ( ss1x::parser::char_p('\\') >>
                //      (   ss1x::parser::char_set_p("\\abfnrtv'\"")
                //       | (ss1x::parser::char_range_p('0', '3') >> ss1x::parser::char_range_p('0', '7') >> ss1x::parser::char_range_p('0', '7'))
                //       | (ss1x::parser::char_range_p('0', '7') || ss1x::parser::char_range_p('0', '7'))
                //       | (ss1x::parser::char_p('x') > (ss1x::parser::xdigit_p || ss1x::parser::xdigit_p))
                //      )
                //     ) [sfunc_escape];
                char escape_char = '\0';
                if (*it_beg == '\\') {
                    if (!::parse_escape(it_beg, it_end, escape_char)) {
                        // std::cout << __func__ << " failed" << std::endl;
                        SSS_POSTION_THROW(std::runtime_error,
                                          "parse_escape error `" << sss::util::make_slice(it_beg, it_end));
                    }
                    content += escape_char;
                }
                else {
                    content += *it_beg++;
                }
            }
        }
        if (is_ok) {
            is_ok = parser_t::parseChar(it_beg, it_end, '"');
        }
        return is_ok;
    }
    bool parse_comma(Iter_t& it_beg, Iter_t it_end)
    {
        return parser_t::parseChar(it_beg, it_end, ',');
    }
    bool parse_rule(const std::string& line, std::string& key, std::string& value)
    {
        Iter_t it_beg = line.begin();
        Iter_t it_end = line.end();
        bool is_ok =
            ::skip_space(it_beg, it_end) &&
            ::parse_dq_str(it_beg, it_end, key) &&
            ::skip_space(it_beg, it_end) &&
            ::parse_comma(it_beg, it_end) &&
            ::skip_space(it_beg, it_end) &&
            ::parse_dq_str(it_beg, it_end, value);
        return is_ok;
    }
} // namespace 

ByteStreamEditor::ByteStreamEditor()
{
}

ByteStreamEditor::ByteStreamEditor(const std::string& rule_path)
{
    this->load(rule_path);
}

void ByteStreamEditor::load(const std::string& rule_path)
{
    // std::cout << __func__ << " `" << rule_path << "`" << std::endl;
    std::string line;
    std::ifstream ifs(rule_path, std::ios_base::in);
    if (!ifs.good()) {
        SSS_POSTION_THROW(std::runtime_error,
                          "unable to read rule file `" << rule_path << "`");
    }

    while (std::getline(ifs, line)) {
        std::string key;
        std::string value;
        if (::parse_rule(line, key, value)) {
            this->add_rule(key, value);
        }
    }
}

void ByteStreamEditor::translate(const std::string& src, const std::string& out, bool replace)
{
    if (replace) {
        std::cout << __func__ << " and replace localy `" << src << "`" << std::endl;
    }
    else {
        std::cout << __func__ << " from `" << src << "` to `" << out << "`" << std::endl;
    }
    std::ifstream ifs(src, std::ios_base::in | std::ios_base::binary);
    if (!ifs.good()) {
        SSS_POSTION_THROW(std::runtime_error,
                          "unable to open file `" << src << "` to read");
    }
    if (replace) {
        std::ostringstream oss;
        this->m_sm.translate(ifs, oss);
        std::ofstream ofs(src, std::ios_base::out | std::ios_base::binary);
        if (!ofs.good()) {
            SSS_POSTION_THROW(std::runtime_error,
                              "unable to open file `" << src << "` to write");
        }
        ofs << oss.str();
    }
    else {
        std::ofstream ofs(out, std::ios_base::out | std::ios_base::binary);
        if (!ofs.good()) {
            SSS_POSTION_THROW(std::runtime_error,
                              "unable to open file `" << out << "` to write");
        }

        this->m_sm.translate(ifs, ofs);
    }
}

void ByteStreamEditor::add_rule(const std::string& key, const std::string& value)
{
    // std::cout << __func__ << " " << VALUE_MSG(key) << " " << VALUE_MSG(value) << std::endl;
    size_t st_id = 0;
    for (size_t i = 0; i < key.length(); ++i) {
        if (i == key.length() - 1) {
            st_id = this->m_sm.ensure_jump(st_id, key[i],
                                           std::bind([=](const std::string& value)->std::string {
                                               return value;
                                           }, value));
            // std::cout << __func__ << ":" << __LINE__ << ":" << st_id << ",`" << value << "`" << std::endl;
        }
        else {
            st_id = this->m_sm.ensure_jump(st_id, key[i]);
            // std::cout << __func__ << ":" << __LINE__ << ":" << st_id << std::endl;
        }
    }
}
