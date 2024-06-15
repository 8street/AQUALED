
#include "text.h"

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
    int plus_pos = ret_str.indexOf("+");
    while (plus_pos >= 0)
    {
        ret_str.setCharAt(plus_pos, ' ');
        plus_pos = ret_str.indexOf("+");
    }
    return ret_str;
}