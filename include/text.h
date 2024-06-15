#pragma once
#ifndef TEXT_H
#    define TEXT_H

#    include "WString.h"

String get_middle_string(const String &str, const String &before, const String &after);
String change_plus_to_space(const String &str);

#endif