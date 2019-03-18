//
// Created by Edward Hyde on 17/03/2019.
//

#ifndef CPP_HTTP_RANGE_FILESERVER_HTTPUTILS_H
#define CPP_HTTP_RANGE_FILESERVER_HTTPUTILS_H

#include <string>
#include <regex>

namespace httputils {
    static std::string replaceAll(const std::string& s,const std::string& rgx_str, const std::string& target){
        std::regex e (rgx_str);
        return std::regex_replace(s, e, target);
    }

    static void split(const std::string& s, std::vector<std::string>& target, const std::string rgx_str = "\\s+") {
        std::regex rgx (rgx_str);
        std::sregex_token_iterator iter(s.begin(), s.end(), rgx, -1);
        std::sregex_token_iterator end;
        while (iter != end)  {
            target.push_back(*iter++);
        }
    }

    static bool accepts(std::string& acceptHeader, std::string& toAccept){
        std::vector<std::string> acceptValues;
        split(acceptHeader, acceptValues,  "\\s*(,|;)\\s*");
        return (std::find(acceptValues.begin(), acceptValues.end(), toAccept) != acceptValues.end()) ||
               (std::find(acceptValues.begin(), acceptValues.end(), replaceAll(toAccept, "/.*$", "/*")) != acceptValues.end()) ||
               (std::find(acceptValues.begin(), acceptValues.end(), "*/*") != acceptValues.end());
    }

    static bool matches(std::string& matchHeader, std::string& toMatch) {
        std::vector<std::string> matchValues;
        split(matchHeader, matchValues, "\\s*,\\s*");
        return (std::find(matchValues.begin(), matchValues.end(), toMatch) != matchValues.end())
               || (std::find(matchValues.begin(), matchValues.end(), "*") != matchValues.end());
    }
}

#endif //CPP_HTTP_RANGE_FILESERVER_HTTPUTILS_H
