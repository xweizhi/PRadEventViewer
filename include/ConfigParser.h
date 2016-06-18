#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <queue>
#include <typeinfo>

// demangle type name
#ifdef __GNUG__
#include <cstdlib>
#include <memory>
#include <cxxabi.h>
// gnu compiler needs to demangle type info
static std::string demangle(const char* name)
{

    int status = 0;

    //enable c++11 by passing the flag -std=c++11 to g++
    std::unique_ptr<char, void(*)(void*)> res {
        abi::__cxa_demangle(name, NULL, NULL, &status),
        std::free
    };

    return (status==0) ? res.get() : name ;
}
#else
// do nothing if not gnu compiler
static std::string demangle(const char* name)
{
    return name;
}
#endif

// config value class
class ConfigValue
{
public:
    std::string _value;

    ConfigValue()
    : _value("0")
    {};

    ConfigValue(const std::string &value);
    ConfigValue(const int &value);
    ConfigValue(const long &value);
    ConfigValue(const long long &value);
    ConfigValue(const unsigned &value);
    ConfigValue(const unsigned long &value);
    ConfigValue(const unsigned long long &value);
    ConfigValue(const float &value);
    ConfigValue(const double &value);
    ConfigValue(const long double &value);

    template<typename T>
    T Convert()
    {
        std::stringstream iss(_value);
        T _cvalue;

        if(!(iss>>_cvalue)) {
            std::cerr << "Config Value Warning: Undefined value returned, failed to convert "
                      <<  _value
                      << " to "
                      << demangle(typeid(T).name())
                      << std::endl;
        }

        return _cvalue;
    };

    char Char();
    unsigned char UChar();
    short Short();
    unsigned short UShort();
    int Int();
    unsigned int UInt();
    long Long();
    long long LongLong();
    unsigned long ULong();
    unsigned long long ULongLong();
    float Float();
    double Double();
    long double LongDouble();
    const char *c_str();
    std::string String() {return _value;};

    operator std::string() const
    {
        return _value;
    };

    bool operator ==(const std::string &rhs) const
    {
        return _value == rhs;
    }
};

// show string content of the config value to ostream
std::ostream &operator << (std::ostream &os, ConfigValue &b);

// config parser class
class ConfigParser
{
public:
    ConfigParser(const std::string &s = " ,\t",  // splitters
                 const std::string &w = " \t",  // white_space
                 const std::vector<std::string> &c = {"#", "//"}); // comment_mark
    virtual ~ConfigParser();
    void SetSplitters(const std::string &s) {splitters = s;};
    void SetWhiteSpace(const std::string &w) {white_space = w;};
    void SetCommentMarks(const std::vector<std::string> &c) {comment_marks = c;};
    void AddCommentMark(const std::string &c);
    void RemoveCommentMark(const std::string &c);
    void EraseCommentMarks();
    bool OpenFile(const std::string &path);
    void CloseFile();
    void OpenBuffer(char *);
    void ClearBuffer();
    bool ParseLine();
    void ParseLine(const std::string &line);
    size_t NbofElements() {return elements.size();};
    std::string GetLine();
    ConfigValue TakeFirst();
    std::vector<ConfigValue> TakeAll();

private:
    std::string splitters;
    std::string white_space;
    std::vector<std::string> comment_marks;
    std::queue<std::string> lines;
    std::queue<std::string> elements;
    std::ifstream infile;

private:
    std::string comment_out(const std::string &str, size_t index = 0);

public:
    static std::string comment_out(const std::string &str, const std::string &c);
    static std::string trim(const std::string &str, const std::string &w);
    static std::queue<std::string> split(const std::string &str, const std::string &s);
    static int find_integer(const std::string &str, const size_t &pos = 0);
    static std::vector<int> find_integers(const std::string &str);
    static void find_integer_helper(const std::string &str, std::vector<int> &result);
};


#endif
