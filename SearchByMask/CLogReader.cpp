#include "CLogReader.h"

#include <stdio.h>

static VOID CALLBACK FileIOCompletionRoutine (
    __in  DWORD dwErrorCode,
    __in  DWORD dwNumberOfBytesTransfered,
    __in  LPOVERLAPPED lpOverlapped )
{
    printf("Error code:\t%x\n", dwErrorCode);
}

bool CLogReader::Open(const char* filePath)
{
    _hFile = CreateFile(filePath,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
        NULL);

    if (_hFile == INVALID_HANDLE_VALUE) { 
        printf("Terminal failure: unable to open file %s for read.\n", filePath);
        return false; 
    }

    return true;
}

void CLogReader::Close()
{
    CloseHandle(_hFile);
    DeleteBuffer();
}

bool CLogReader::SetFilter(const char *filter)
{
    if (filter == '\0') {
        printf("Filter is not set");
        return false;
    }
    _filter = filter;
    return true;
}

bool CLogReader::GetNextLine(char *buf, const int bufsize)
{
    if (!_filter) {
        printf("Filter is not set");
        return false;
    }

    if (_needToRead) {
        if (!ReadData(bufsize))
            return false;
        _needToRead = false;
    }

    while (_bufferIdx < _symbolsRead) {
        int startLineIdx = _bufferIdx;
        bool res = IsMatchedLine(0);
        while (!IsEndOfLine(_bufferIdx))
            _bufferIdx++;
        int endLineIdx = _bufferIdx;
        _bufferIdx++;
        if (res) {
            memset(buf, 0, LINE_BUFFERSIZE);
            int lineSize = endLineIdx - startLineIdx + 1;
            if (lineSize > LINE_BUFFERSIZE)
                lineSize = LINE_BUFFERSIZE;
            for (int i = 0; i < lineSize; ++i) {
                int bufferIdx = i + startLineIdx;
                if (IsEndOfLine(bufferIdx))
                    break;
                buf[i] = _readBuffer[i + startLineIdx];
            }
            return true;
        }
    }

    //There could be a case when it has reached the end of the buffer, but the last line wasn't read completely,
    //so we have to read data from the start of this line
    if (_bufferIdx >= _symbolsRead) {
        _bufferIdx = _symbolsRead - 1;
        while (_bufferIdx > 0) {
            if (IsEndOfLine(_bufferIdx))
                break;
            _bufferIdx--;
        }
        if (_bufferIdx == 0)
            return false;
        _bufferIdx++;
        _needToRead = true;
        return GetNextLine(buf, bufsize);
    }

    return false;
}

bool CLogReader::ReadData(const int bufsize)
{
    DeleteBuffer();
    _readBuffer = new char[bufsize];

    _overlapped.Offset += _bufferIdx;
    _bufferIdx = 0;
    DWORD  dwBytesRead = 0;

    if (FALSE == ReadFileEx(_hFile, _readBuffer, bufsize, &_overlapped, FileIOCompletionRoutine))
    {
        printf("Terminal failure: Unable to read from file.\n GetLastError=%08x\n", GetLastError());
        Close();
        return false;
    }

    _symbolsRead = _overlapped.InternalHigh;
    return _symbolsRead > 0;
}

bool CLogReader::IsMatchedLine(int filterIdx)
{
    if (IsEndOfLine(_bufferIdx)) {
        if (_filter[filterIdx] == '\0')
            return true;
        while (_filter[filterIdx] != '\0') {
            if (_filter[filterIdx] != '*')
                return false;
            filterIdx++;
        }
        return true;
    }
    if (_filter[filterIdx] == '\0') {
        return IsEndOfLine(_bufferIdx);
    }

    if (_filter[filterIdx] == '*') {
        filterIdx++;
        if (_filter[filterIdx] == '\0')
            return true;
        int lineTempIdx = _bufferIdx;
        while (!IsEndOfLine(_bufferIdx)) {
            if (IsMatchedLine(filterIdx)) {
                return true;
                break;
            }
            lineTempIdx++;
        }
        _bufferIdx = lineTempIdx;
        return false;
    }
    else if (_filter[filterIdx] == '?') {
        if (IsEndOfLine(_bufferIdx))
            return false;
        filterIdx++;
        _bufferIdx++;
        return IsMatchedLine(filterIdx);
    }
    else if (_filter[filterIdx] == _readBuffer[_bufferIdx]) {
        filterIdx++;
        _bufferIdx++;
        return IsMatchedLine(filterIdx);
    }
    _bufferIdx++;
    return false;
}

bool CLogReader::IsEndOfLine(int bufferIdx) const
{
    return bufferIdx >= _symbolsRead ||
        _readBuffer[bufferIdx] == '\0' ||
        _readBuffer[bufferIdx] == '\r' ||
        _readBuffer[bufferIdx] == '\n' ||
        (int)_readBuffer[bufferIdx] < 0;
}

inline void CLogReader::DeleteBuffer()
{
    if (_readBuffer) {
        delete[] _readBuffer;
        _readBuffer = 0;
    }
}

