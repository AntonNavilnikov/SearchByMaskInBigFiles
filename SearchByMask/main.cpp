#include "CLogReader.h"

#include <stdio.h>


int main(int argc, char** argv)
{
    if (argc < 3) {
        printf("too few of parameters (passed: %i, required: 2). Usage:\n", argc);
        printf("SearchByMask.exe <log_file> <filter>\n");
    }

    const char* filePath = argv[1];
    const char* filter = argv[2];

    CLogReader cLogReader;
    if (!cLogReader.SetFilter(filter)) {
        printf("filter %s is not valid", filter);
        return 1;
    }

    if (!cLogReader.Open(filePath)) {
        printf("path to file %s is not valid", filePath);
        return 1;
    }

    printf("Search result:\n");
    char buf[LINE_BUFFERSIZE] = { 0 };
    while (cLogReader.GetNextLine(buf, BUFFERSIZE)) {
        printf("%s\n", buf);
    }
    
    cLogReader.Close();

    return 0;
}
