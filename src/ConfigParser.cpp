//============================================================================//
// A simple parser class to read text file                                    //
//                                                                            //
// Chao Peng                                                                  //
// 06/07/2016                                                                 //
//============================================================================//

#include "ConfigParser.h"
#include <cstring>

using namespace std;

// Config Value
std::ostream &operator << (std::ostream &os, ConfigValue &b)
{
    return  os << b._value;
};


ConfigValue::ConfigValue(const string &value)
: _value(value)
{}

ConfigValue::ConfigValue(const int &value)
: _value(to_string(value))
{}

ConfigValue::ConfigValue(const long &value)
: _value(to_string(value))
{}

ConfigValue::ConfigValue(const long long &value)
: _value(to_string(value))
{}

ConfigValue::ConfigValue(const unsigned &value)
: _value(to_string(value))
{}

ConfigValue::ConfigValue(const unsigned long &value)
: _value(to_string(value))
{}

ConfigValue::ConfigValue(const unsigned long long &value)
: _value(to_string(value))
{}

ConfigValue::ConfigValue(const float &value)
: _value(to_string(value))
{}

ConfigValue::ConfigValue(const double &value)
: _value(to_string(value))
{}

ConfigValue::ConfigValue(const long double &value)
: _value(to_string(value))
{}

char ConfigValue::Char()
{
    return (char)stoi(_value);
}

unsigned char ConfigValue::UChar()
{
    return (unsigned char)stoul(_value);
}

short ConfigValue::Short()
{
    return (short)stoi(_value);
}

unsigned short ConfigValue::UShort()
{
    return (unsigned short)stoul(_value);
}

int ConfigValue::Int()
{
    return stoi(_value);
}

unsigned int ConfigValue::UInt()
{
    return (unsigned int)stoul(_value);
}

long ConfigValue::Long()
{
    return stol(_value);
}

long long ConfigValue::LongLong()
{
    return stoll(_value);
}

unsigned long ConfigValue::ULong()
{
    return stoul(_value);
}

unsigned long long ConfigValue::ULongLong()
{
    return stoull(_value);
}

float ConfigValue::Float()
{
    return stof(_value);
}

double ConfigValue::Double()
{
    return stod(_value);
}

long double ConfigValue::LongDouble()
{
    return stold(_value);
}

const char *ConfigValue::c_str()
{
    return _value.c_str();
}


// Config Parser
ConfigParser::ConfigParser(const string &s,
                           const string &w,
                           const vector<string> &c)
: splitters(s), white_space(w), comment_marks(c)
{
}

ConfigParser::~ConfigParser()
{
    CloseFile();
}

void ConfigParser::AddCommentMarks(const string &c)
{
    comment_marks.push_back(c);
}

bool ConfigParser::OpenFile(const string &path)
{
    infile.open(path);
    return infile.is_open();
}

void ConfigParser::CloseFile()
{
    infile.close();
}

void ConfigParser::OpenBuffer(char *buf)
{
    string buffer = buf;

    string line;
    for(auto c : buffer)
    {
        if(c != '\n') {
            line += c;
        } else {
            lines.push(line);
            line = "";
        }
    }
}

void ConfigParser::ClearBuffer()
{
    queue<string>().swap(lines);
}

string ConfigParser::GetLine()
{
    if(lines.size()) {
        string out = lines.front();
        lines.pop();
        return out;
    }

    return "";
}

bool ConfigParser::ParseLine()
{
    if(infile.is_open()) {
        string line;
        bool not_end = getline(infile, line);
        if(not_end) {
            ParseLine(line);
        }
        return not_end;
    } else {
        if(!lines.size())
            return false;
        ParseLine(lines.front());
        lines.pop();
        return true;
    }
}

void ConfigParser::ParseLine(const string &line)
{
    string trim_line = trim(comment_out(line), white_space);
    elements = split(trim_line, splitters);
}

ConfigValue ConfigParser::TakeFirst()
{
    if(elements.empty())
        return ConfigValue("0");
    string output = elements.front();
    elements.pop();

    return ConfigValue(output);
}

vector<ConfigValue> ConfigParser::TakeAll()
{
    vector<ConfigValue> output;

    while(elements.size())
    {
        output.push_back(ConfigValue(elements.front()));
        elements.pop();
    }

    return output;
}

string ConfigParser::comment_out(const string &str, size_t index)
{
    if(index >= comment_marks.size())
        return str;
    else {
        string str_co = comment_out(str, comment_marks.at(index));
        return comment_out(str_co, ++index);
    }
        
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

queue<string> ConfigParser::split(const string &str, const string &s)
{
    queue<string> eles;

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

    return eles;
}
