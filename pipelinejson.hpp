#ifndef PIPELINEJSON_H
#define PIPELINEJSON_H
#include <string>

class pipelineJson
{
public:
    pipelineJson();

    static std::string basicReaderWriter(char* inFile, char* outFile);

};

#endif // PIPELINEJSON_H
