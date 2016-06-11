#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <fstream>
#include <string>
#include <vector>
#include <queue>

class ConfigParser
{
public:
    ConfigParser(const std::string &s = " ,\t",  // splitters
                 const std::string &w = " \t",  // white_space
                 const std::vector<std::string> &c = {"#", "//"}); // comment_mark
    virtual ~ConfigParser();
    void AddCommentMarks(const std::string &c);
    bool OpenFile(const std::string &path);
    void CloseFile();
    void OpenBuffer(char *);
    void ClearBuffer();
    bool ParseLine();
    void ParseLine(const std::string &line);
    size_t NbofElements() {return elements.size();};
    std::string GetLine();
    std::string TakeFirst();
    std::vector<std::string> TakeAll();

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
};


#endif
