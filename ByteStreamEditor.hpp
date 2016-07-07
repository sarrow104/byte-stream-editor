#ifndef __BYTESTREAMEDITOR_HPP_1467685696__
#define __BYTESTREAMEDITOR_HPP_1467685696__

#include <string>

#include "SequenceSM.hpp"

class ByteStreamEditor
{
public:
    ByteStreamEditor();
    explicit ByteStreamEditor(const std::string& rule_path);
    ~ByteStreamEditor() = default;

public:
    ByteStreamEditor(ByteStreamEditor&& ) = default;
    ByteStreamEditor& operator = (ByteStreamEditor&& ) = default;

public:
    ByteStreamEditor(const ByteStreamEditor& ) = default;
    ByteStreamEditor& operator = (const ByteStreamEditor& ) = default;

public:
    void load(const std::string& rule_path);
    void translate(const std::string& src, const std::string& out, bool replace = false);
    void add_rule(const std::string& key, const std::string& value);

private:
    SequenceSM m_sm;
};


#endif /* __BYTESTREAMEDITOR_HPP_1467685696__ */
