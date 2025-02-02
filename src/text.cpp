#include <vector>

#include "text.h"
#include <Arduino.h>


String get_middle_string(const String &str, const String &before, const String &after)
{
    const int pos1 = str.indexOf(before) + before.length();
    const int pos2 = str.indexOf(after);
    if (pos1 < 0 || pos2 < 0)
    {
        return str;
    }
    return str.substring(pos1, pos2);
}

String change_plus_to_space(const String &str)
{
    String ret_str = str;
    ret_str.replace('+', ' ');
    return ret_str;
}

String change_browser_code_to_symbol(const String &str)
{
    String ret_str = str;
    std::vector<std::pair<String, String>> arr;
    arr.emplace_back(std::make_pair("%20", "")); // delete spaces
    arr.emplace_back(std::make_pair("%23", "#"));
    arr.emplace_back(std::make_pair("%24", "$"));
    arr.emplace_back(std::make_pair("%25", "%"));
    arr.emplace_back(std::make_pair("%26", "&"));
    arr.emplace_back(std::make_pair("%27", "'"));
    arr.emplace_back(std::make_pair("%2B", "+"));
    arr.emplace_back(std::make_pair("%2C", ","));
    arr.emplace_back(std::make_pair("%2F", "/"));
    arr.emplace_back(std::make_pair("%3A", ":"));
    arr.emplace_back(std::make_pair("%3B", ";"));
    arr.emplace_back(std::make_pair("%3D", "="));
    arr.emplace_back(std::make_pair("%3F", "?"));
    arr.emplace_back(std::make_pair("%5B", "["));
    arr.emplace_back(std::make_pair("%5C", "\\"));
    arr.emplace_back(std::make_pair("%5D", "]"));
    arr.emplace_back(std::make_pair("%5E", "^"));
    arr.emplace_back(std::make_pair("%60", "`"));
    arr.emplace_back(std::make_pair("%7C", "|"));

    for (const auto &p : arr)
    {
        yield();
        ret_str.replace(p.first, p.second);
    }
    return ret_str;
}