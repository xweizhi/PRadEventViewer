#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <fstream>
#include <string>
#include <queue>

class ConfigParser
{
public:
    ConfigParser(const std::string &s = " ,.\t",  // splitters
                 const std::string &w = " \t",  // white_space
                 const std::string &c = "#"); // comment_mark
    virtual ~ConfigParser();
    void OpenFile(const std::string &path);
    void Close();
    bool ParseLine();
    void ParseLine(const std::string &line);
    std::string TakeFirst();

private:
    std::string splitters;
    std::string white_space;
    std::string comment_mark;
    std::queue<std::string> elements;
    std::ifstream infile;

private:
    std::string comment_out(const std::string &str, const std::string &c);
    std::string trim(const std::string &str, const std::string &w);
    void split(std::queue<std::string> &eles, const std::string &str, const std::string &s);
};


#endif
