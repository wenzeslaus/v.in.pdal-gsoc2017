#include <sstream>
#include "pipelinejson.hpp"

using namespace std;


pipelineJson::pipelineJson()
{

}

#pragma GCC diagnostic ignored "-Wwrite-strings"
string pipelineJson::enquote(char* str){
    std::ostringstream s;
    s << "\"" << str << "\"";

    return s.str();

}

string pipelineJson::basicReaderWriter(char* inFile, char* outFile){
    std::ostringstream s;
    s <<
         "{" <<
         " \"pipeline\":[" << endl <<

         "   " << enquote(inFile) << "\"," << endl <<

         "   " << enquote(outFile)  << endl <<

         " ]" << endl <<
         "}";

    return s.str();

}

string pipelineJson::basicVectorMapReaderWriter(char* inFile, char* outFile){
    std::ostringstream s;
    s <<
         "{" <<
         " \"pipeline\":[" << endl <<

         "   " << enquote(inFile) << //"," << endl <<

          " ]" << endl <<
         "}";

    return s.str();

}

#pragma GCC diagnostic pop
