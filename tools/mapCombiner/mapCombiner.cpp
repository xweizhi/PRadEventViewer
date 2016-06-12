#include <string>
#include <vector>
#include <queue>
#include <iostream>
#include <iomanip>
#include <unordered_map>
#include "ConfigParser.h"

using namespace std;

int main(int argc, char *argv[])
{
    char *ptr;
    string output = "combined_map.txt", input;
    queue<string> input_list;

    for(int i = 1; i < argc; ++i)
    {
        ptr = argv[i];
        if(*(ptr++) == '-') {
            switch(*(ptr++))
            {
            case 'o':
                output = argv[++i];
                break;
            case 'i':
                for(int j = i + 1; j < argc; ++j)
                {
                    if(*argv[j] == '-') {
                        i = j;
                        break;
                    }
                    input = argv[j];
                    if(!input.empty())
                        input_list.push(input);
                }
                break;
            default:
                printf("Unkown option!\n");
                exit(1);
            }
        }
    }
    ConfigParser parser;
    unordered_map<string, vector<string>> eles_map;

    if(input_list.size() < 2) {
        cerr << "Less than 2 input files, no need to combine" << endl;
    }

    parser.OpenFile(input_list.front());
    input_list.pop();

    // read tdc map
    while(parser.ParseLine())
    {
        if(!parser.NbofElements())
            continue;
        string key = parser.TakeFirst();
        vector<string> ori_eles = parser.TakeAll();

        eles_map[key] = ori_eles;
    }

    parser.CloseFile();

    while(input_list.size())
    {
        parser.OpenFile(input_list.front());
        input_list.pop();
        while(parser.ParseLine())
        {
            if(!parser.NbofElements())
                continue;
            string key = parser.TakeFirst();
            auto it = eles_map.find(key);
            if(it == eles_map.end()) {
                cout << "Cannot find key " << key << " in original map" << endl;
                continue;
            }
            vector<string> new_eles = parser.TakeAll();
            it->second.insert(it->second.end(), new_eles.begin(), new_eles.end());
        }
        parser.CloseFile();
    }

    ofstream outfile(output);
    for(auto ele : eles_map)
    {
        outfile << setw(12) << ele.first;
        vector<string> eles = ele.second;
        for(auto e : eles)
        {
            outfile << setw(12) << e;
        }
        outfile << endl;
    }

    outfile.close();
    return 0;
}
