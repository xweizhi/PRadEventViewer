#include "ConfigParser.h"
#include <cstring>

using namespace std;

ConfigParser::ConfigParser(const string &s, const string &w, const string &c)
: splitters(s), white_space(w), comment_mark(c)
{

}

ConfigParser::~ConfigParser()
{

}

void ConfigParser::OpenFile(const string &path)
{
    infile.open(path);
}


void ConfigParser::Close()
{
    infile.close();
}


bool ConfigParser::ParseLine()
{
    string line;
    bool not_end = getline(infile, line);
    if(not_end) {
        ParseLine(line);
    }
    return not_end;
}

void ConfigParser::ParseLine(const string &line)
{
    queue<string>().swap(elements);
    string trim_line = trim(comment_out(line, comment_mark), white_space);
    split(elements, trim_line, splitters);
}

string ConfigParser::TakeFirst()
{
    if(elements.empty())
        return "";

     string output = elements.front();
     elements.pop();

     return output;
}

string ConfigParser::comment_out(const string &str,
                   const string &c)
{
    if(c.empty()) {
        return str;
    }

    const auto commentBegin = str.find(c);
    return str.substr(0, commentBegin);
}


string ConfigParser::trim(const string &str,
            const string &w)
{

    const auto strBegin = str.find_first_not_of(w);
    if (strBegin == string::npos)
        return ""; // no content

    const auto strEnd = str.find_last_not_of(w);

    const auto strRange = strEnd - strBegin + 1;

    return str.substr(strBegin, strRange);
}

void ConfigParser::split(queue<string> &eles, const string &str, const string &s)
{
    char *cstr = new char[str.length() + 1];

    strcpy(cstr, str.c_str());

    char *pch = strtok(&cstr[0], s.c_str());
    string ele;

    while(pch != nullptr)
    {
        ele = pch;
        eles.push(pch);
        pch = strtok(nullptr, s.c_str());
    }

    delete cstr;
}
