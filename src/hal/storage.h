#pragma once
#include <SD.h>
#include <FS.h>
#include "../config.h"

class Storage {
public:
    static Storage& instance();
    bool    begin();
    bool    mounted() const { return _mounted; }

    // Directory listing — fills names[], returns count
    int     listDir(const char* path, String* names, int maxNames, const char* ext = nullptr);
    bool    exists(const char* path);
    size_t  fileSize(const char* path);
    File    open(const char* path, const char* mode = "r");

    // Simple helpers
    String  readText(const char* path);
    bool    writeText(const char* path, const String& data);
    bool    ensureDir(const char* path);

    uint64_t totalBytes();
    uint64_t usedBytes();

private:
    Storage() = default;
    bool _mounted = false;
};
