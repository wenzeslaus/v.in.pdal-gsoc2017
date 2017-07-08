#ifndef PIPELINEJSON_H
#define PIPELINEJSON_H
#include <string>

class pipelineJson
{
public:
    pipelineJson();

    static std::string basicReaderWriter(char* inFile, char* outFile);
    static std::string basicVectorMapReaderWriter(char* inFile, char* outFile);

private:
    static std::string enquote(char* str);

};

#endif // PIPELINEJSON_H
