#define BUFSIZE 512

__declspec(dllimport) unsigned long __stdcall
GetEnvironmentVariableA(const char* name, char* buf, unsigned long size);

    char*
api_getenv(char* name)
{
    unsigned long ret;
    static char buf[BUFSIZE];

    ret = GetEnvironmentVariableA(name, buf, BUFSIZE);
    return (0 < ret && ret < BUFSIZE) ? buf : 0;
}
