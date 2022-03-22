#pragma once

#include <windows.h>

#define BUFFERSIZE 100000
#define LINE_BUFFERSIZE 200


class CLogReader
{
public:
    CLogReader() : _filter(0), _bufferIdx(0), _needToRead(true)
    {
    }

    ~CLogReader()
    {
        DeleteBuffer();
    }

    bool Open(const char* filePath);
    void Close();
    bool SetFilter(const char *filter);
    bool GetNextLine(char *buf, const int bufsize);

private:
    bool ReadData(const int bufsize);
    bool IsMatchedLine(int filterIdx);
    bool IsEndOfLine(int bufferIdx) const;
    inline void DeleteBuffer();

    
    const char *_filter;
    HANDLE _hFile;
    OVERLAPPED _overlapped = { 0 };

    char* _readBuffer;
    int _bufferIdx;
    bool _needToRead;
    ULONG_PTR _symbolsRead;
};

