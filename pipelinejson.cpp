#include <sstream>
#include "pipelinejson.hpp"

using namespace std;


pipelineJson::pipelineJson()
{

}


string pipelineJson::basicReaderWriter(char* inFile, char* outFile){
    std::ostringstream s;
    s <<
         "{" <<
          " \"pipeline\":[" <<

            "\"" << inFile << "\"," <<

         "\"" << outFile << "\"" <<

          " ]" <<
         "}";

    return s.str();

}
