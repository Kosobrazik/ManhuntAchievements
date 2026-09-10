#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <cstdio>
#include <cstdint>
#include <string>
#include <vector>
#include <filesystem>

struct Module { uintptr_t base; DWORD size; std::wstring path; };
struct Breakpoint { uintptr_t address; BYTE original; const char* name; bool active; };
static FILE* report;
static std::vector<Module> modules;
static void Address(uintptr_t address)
{
    fprintf(report, "%08lX", static_cast<unsigned long>(address));
    for (const auto& m : modules)
        if (address >= m.base && address - m.base < m.size) {
            fprintf(report, " (%ls+%lX)", m.path.c_str(), static_cast<unsigned long>(address-m.base));
            break;
        }
}
static void Stack(HANDLE process, DWORD threadId, CONTEXT* saved = nullptr)
{
    HANDLE thread = OpenThread(THREAD_GET_CONTEXT | THREAD_SET_CONTEXT, FALSE, threadId);
    CONTEXT ctx{}; ctx.ContextFlags = CONTEXT_FULL;
    if (thread && GetThreadContext(thread, &ctx)) {
        fprintf(report, "EIP="); Address(ctx.Eip);
        fprintf(report, " ESP=%08lX EBP=%08lX EAX=%08lX ECX=%08lX EDX=%08lX\n",
            ctx.Esp, ctx.Ebp, ctx.Eax, ctx.Ecx, ctx.Edx);
        if (saved) *saved = ctx;
        if (ctx.Eip == 0x5E27D9 || ctx.Eip == 0x5F6B33) {
            DWORD pointer=0; char filename[512]{}; SIZE_T count;
            ReadProcessMemory(process,reinterpret_cast<void*>(0x7397AC),&pointer,4,&count);
            ReadProcessMemory(process,reinterpret_cast<void*>(pointer),filename,sizeof(filename)-1,&count);
            fprintf(report,"FONT FILENAME=%s\n",filename);
        }
        if (ctx.Eip == 0x5EE338) {
            DWORD pointer=0; char filename[512]{}; SIZE_T count;
            ReadProcessMemory(process,reinterpret_cast<void*>(0x7397A4),&pointer,4,&count);
            ReadProcessMemory(process,reinterpret_cast<void*>(pointer),filename,sizeof(filename)-1,&count);
            fprintf(report,"FRONTEND TXD FILENAME=%s\n",filename);
        }
        DWORD words[80]{}; SIZE_T read = 0;
        ReadProcessMemory(process, reinterpret_cast<void*>(ctx.Esp), words, sizeof(words), &read);
        for (size_t i=0; i<read/sizeof(DWORD); ++i) {
            fprintf(report, "  stack+%03X ", static_cast<unsigned>(i*4)); Address(words[i]); fputc('\n', report);
        }
    }
    if (thread) CloseHandle(thread);
    fflush(report);
}
static void AddModule(HANDLE process, HANDLE file, void* base)
{
    wchar_t path[1024]{};
    if (file) GetFinalPathNameByHandleW(file, path, 1024, FILE_NAME_NORMALIZED);
    IMAGE_DOS_HEADER dos{}; IMAGE_NT_HEADERS nt{}; SIZE_T read;
    ReadProcessMemory(process, base, &dos, sizeof(dos), &read);
    if (dos.e_magic == IMAGE_DOS_SIGNATURE)
        ReadProcessMemory(process, static_cast<BYTE*>(base)+dos.e_lfanew, &nt, sizeof(nt), &read);
    modules.push_back({reinterpret_cast<uintptr_t>(base), nt.OptionalHeader.SizeOfImage, path});
    fprintf(report, "MODULE %p size=%08lX %ls\n", base, nt.OptionalHeader.SizeOfImage, path);
    fflush(report);
    if (file) CloseHandle(file);
}
int wmain(int argc, wchar_t** argv)
{
    if (argc != 3) { puts("Usage: TraceStartup.exe path-to-manhunt.exe report.txt"); return 2; }
    if (_wfopen_s(&report, argv[2], L"w") || !report) return 3;
    static_assert(sizeof(void*) == 4, "Build Win32 to inspect Manhunt's x86 context");
    STARTUPINFOW startup{}; startup.cb=sizeof(startup); PROCESS_INFORMATION pi{};
    auto cwd=std::filesystem::path(argv[1]).parent_path().wstring();
    std::wstring command=L"\""+std::wstring(argv[1])+L"\"";
    if (!CreateProcessW(argv[1], command.data(), nullptr, nullptr, FALSE,
        DEBUG_ONLY_THIS_PROCESS, nullptr, cwd.c_str(), &startup, &pi)) {
        fprintf(report, "CreateProcess failed: %lu\n", GetLastError()); fclose(report); return 4;
    }
    DebugSetProcessKillOnExit(FALSE);
    fprintf(report, "PID=%lu; external startup trace, timeout=45 seconds\n", pi.dwProcessId);
    CloseHandle(pi.hThread);
    std::vector<Breakpoint> breaks;
    bool initialBreak=true, exited=false;
    const auto start=GetTickCount64();
    while (GetTickCount64()-start < 45000) {
        DEBUG_EVENT e{};
        if (!WaitForDebugEvent(&e, 250)) continue;
        DWORD disposition=DBG_CONTINUE;
        if (e.dwDebugEventCode==CREATE_PROCESS_DEBUG_EVENT) {
            AddModule(pi.hProcess,e.u.CreateProcessInfo.hFile,e.u.CreateProcessInfo.lpBaseOfImage);
            CloseHandle(e.u.CreateProcessInfo.hThread);
            CloseHandle(e.u.CreateProcessInfo.hProcess);
        } else if (e.dwDebugEventCode==CREATE_THREAD_DEBUG_EVENT) {
            CloseHandle(e.u.CreateThread.hThread);
        } else if (e.dwDebugEventCode==LOAD_DLL_DEBUG_EVENT) {
            AddModule(pi.hProcess,e.u.LoadDll.hFile,e.u.LoadDll.lpBaseOfDll);
            if (_wcsicmp(std::filesystem::path(modules.back().path).filename().c_str(),L"ntdll.dll")==0) {
                HMODULE local=GetModuleHandleW(L"ntdll.dll");
                for (const char* name : {"RtlExitUserProcess", "NtTerminateProcess"}) {
                    auto function=GetProcAddress(local,name);
                    if (!function) continue;
                    uintptr_t address=modules.back().base+reinterpret_cast<uintptr_t>(function)-reinterpret_cast<uintptr_t>(local);
                    BYTE original=0, trap=0xCC; SIZE_T count=0;
                    if (ReadProcessMemory(pi.hProcess,reinterpret_cast<void*>(address),&original,1,&count) &&
                        WriteProcessMemory(pi.hProcess,reinterpret_cast<void*>(address),&trap,1,&count)) {
                        FlushInstructionCache(pi.hProcess,reinterpret_cast<void*>(address),1);
                        breaks.push_back({address,original,name,true});
                    }
                }
            }
        } else if (e.dwDebugEventCode==EXCEPTION_DEBUG_EVENT) {
            const auto& ex=e.u.Exception;
            bool ours=false;
            if (ex.ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT) {
                for (auto& bp : breaks) if (bp.active && bp.address==reinterpret_cast<uintptr_t>(ex.ExceptionRecord.ExceptionAddress)) {
                    fprintf(report,"BREAK %s thread=%lu\n",bp.name,e.dwThreadId);
                    CONTEXT ctx{}; Stack(pi.hProcess,e.dwThreadId,&ctx);
                    SIZE_T written;
                    WriteProcessMemory(pi.hProcess,reinterpret_cast<void*>(bp.address),&bp.original,1,&written);
                    FlushInstructionCache(pi.hProcess,reinterpret_cast<void*>(bp.address),1);
                    HANDLE thread=OpenThread(THREAD_SET_CONTEXT,FALSE,e.dwThreadId);
                    ctx.Eip=static_cast<DWORD>(bp.address); SetThreadContext(thread,&ctx); CloseHandle(thread);
                    bp.active=false; ours=true; break;
                }
                if (!ours && initialBreak) {
                    initialBreak=false; ours=true;
                    // One-shot checkpoints for the verified 0x400000 Manhunt executable.
                    // Enable only for this local diagnostic session's supported binary.
                    const Breakpoint checkpoints[] = {
                        {0x4C114B,0,"platform init result",false},
                        {0x4C116F,0,"app initialize result",false},
                        {0x4C1205,0,"RegisterClass result",false},
                        {0x4C1378,0,"window created",false},
                        {0x4C13A8,0,"load display settings result",false},
                        {0x4C1486,0,"RenderWare initialize result",false},
                        {0x4D7870,0,"COverlayMgr create result",false},
                        {0x489BF4,0,"frontend overlay initialize result",false},
                        {0x5E27D8,0,"font data load result",false},
                        {0x5F6B32,0,"font data file read result",false},
                        {0x5EE337,0,"frontend TXD load result",false},
                        {0x4D78BA,0,"GLOBAL TOC load result",false},
                        {0x4D7906,0,"PakFile initialize result",false},
                        {0x4C16B0,0,"game initialize result",false},
                        {0x4C17B4,0,"enter main loop",false},
                        {0x4C1A30,0,"leave main loop",false}
                    };
                    for (auto bp : checkpoints) {
                        BYTE trap=0xCC; SIZE_T count;
                        if (ReadProcessMemory(pi.hProcess,reinterpret_cast<void*>(bp.address),&bp.original,1,&count) &&
                            WriteProcessMemory(pi.hProcess,reinterpret_cast<void*>(bp.address),&trap,1,&count)) {
                            FlushInstructionCache(pi.hProcess,reinterpret_cast<void*>(bp.address),1);
                            bp.active=true; breaks.push_back(bp);
                        }
                    }
                }
            }
            if (!ours) {
                fprintf(report,"EXCEPTION %08lX firstChance=%lu thread=%lu at ",ex.ExceptionRecord.ExceptionCode,ex.dwFirstChance,e.dwThreadId);
                Address(reinterpret_cast<uintptr_t>(ex.ExceptionRecord.ExceptionAddress)); fputc('\n',report);
                for (DWORD i=0;i<ex.ExceptionRecord.NumberParameters;++i)
                    fprintf(report,"  parameter[%lu]=%08lX\n",i,static_cast<unsigned long>(ex.ExceptionRecord.ExceptionInformation[i]));
                Stack(pi.hProcess,e.dwThreadId);
                // Windows reports invalid CloseHandle/RegCloseKey to an attached
                // debugger even when the same call simply returns outside it.
                disposition=(ex.ExceptionRecord.ExceptionCode==0xC0000008 && ex.dwFirstChance)
                    ? DBG_CONTINUE : DBG_EXCEPTION_NOT_HANDLED;
            }
        } else if (e.dwDebugEventCode==EXIT_PROCESS_DEBUG_EVENT) {
            fprintf(report,"PROCESS EXIT code=%08lX elapsed=%llu ms\n",e.u.ExitProcess.dwExitCode,GetTickCount64()-start);
            exited=true;
        }
        ContinueDebugEvent(e.dwProcessId,e.dwThreadId,disposition);
        if (exited) break;
    }
    if (!exited) {
        for (const auto& bp : breaks) if (bp.active) {
            SIZE_T written;
            WriteProcessMemory(pi.hProcess,reinterpret_cast<void*>(bp.address),&bp.original,1,&written);
            FlushInstructionCache(pi.hProcess,reinterpret_cast<void*>(bp.address),1);
        }
        DebugActiveProcessStop(pi.dwProcessId);
        fprintf(report,"TIMEOUT: debugger detached; game left running\n");
    }
    CloseHandle(pi.hProcess); fclose(report);
    return 0;
}
