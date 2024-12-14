#pragma once
#include <windows.h>
#include <tchar.h>
#include <stdio.h> 
#include <strsafe.h>
#include <string>

struct Katagomo
{
    HANDLE g_hChildStd_IN_Rd = NULL;
    HANDLE g_hChildStd_IN_Wr = NULL;
    HANDLE g_hChildStd_OUT_Rd = NULL;
    HANDLE g_hChildStd_OUT_Wr = NULL;

    Katagomo() = default;

    void init()
    {
        SECURITY_ATTRIBUTES saAttr;
        saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        saAttr.bInheritHandle = TRUE;
        saAttr.lpSecurityDescriptor = NULL;

        if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0))
            ErrorExit(TEXT("StdoutRd CreatePipe"));
        if (!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))
            ErrorExit(TEXT("Stdout SetHandleInformation"));
        if (!CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0))
            ErrorExit(TEXT("Stdin CreatePipe"));
        if (!SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0))
            ErrorExit(TEXT("Stdin SetHandleInformation"));

        CreateChildProcess();
        Sleep(4000);
        ReadFromPipe();
    }

    void CreateChildProcess()
    {
        TCHAR szCmdline[] = TEXT("D:\\Projects\\Katagomo\\katago\\engine\\freestyle.exe gtp -model D:\\Projects\\Katagomo\\weights\\freestyle.bin.gz -config D:\\Projects\\Katagomo\\katago\\default.cfg");
        PROCESS_INFORMATION piProcInfo;
        STARTUPINFO siStartInfo;
        ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
        ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
        siStartInfo.cb = sizeof(STARTUPINFO);
        siStartInfo.hStdError = g_hChildStd_OUT_Wr;
        siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
        siStartInfo.hStdInput = g_hChildStd_IN_Rd;
        siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

        if (!CreateProcess(NULL, szCmdline, NULL, NULL, TRUE, 0, NULL, NULL, &siStartInfo, &piProcInfo))
            ErrorExit(TEXT("CreateProcess"));

        CloseHandle(piProcInfo.hProcess);
        CloseHandle(piProcInfo.hThread);
        CloseHandle(g_hChildStd_OUT_Wr);
        CloseHandle(g_hChildStd_IN_Rd);
    }

    void WriteToPipe(string input)
    {
        DWORD dwWritten;
        WriteFile(g_hChildStd_IN_Wr, input.c_str(), input.length(), &dwWritten, NULL);
        WriteFile(g_hChildStd_IN_Wr, "\n", 1, &dwWritten, NULL);
    }

    string ReadFromPipe()
    {
        DWORD dwRead;
        CHAR chBuf[65536];
        string result;

        ReadFile(g_hChildStd_OUT_Rd, chBuf, 65536, &dwRead, NULL);

        result.append(chBuf, dwRead);
        std::cout << result;

        return result;
    }

    string loc_to_str(int y, int x)
    {
        if (x >= 9)
            return (char)('A' + x) + std::to_string(y);
        else
            return (char)('A' + x - 1) + std::to_string(y);
    }

    auto str_to_mov(string s, Board state)
    {
        int x = s[0] - 'A' + 1;
        int y = 0;
        if (x >= 10) 
            x--;
        if (s[2] == ' ')
            y = s[1] - '1' + 1;
        else
            y = std::stoi(s.substr(1, 2));
        return make_pair(y, x);
    }

    double str_to_val(string s)
    {
        int val = std::stoi(s.substr(0, s.find(" ")));
        return (1.0 * val / 10000 - 0.5) * 2;
    }

    void clear()
    {
        WriteToPipe("clear_board");
        Sleep(10);
    }

    void undo()
    {
        WriteToPipe("undo");
        Sleep(10);
    }

    auto get_move_list(Board state, string tops)
    {
        vector <pair<short, short>> v_mov;
        if (state.lst_mov.first == 0)
        {
            state.resize();
        
            for (auto item : state.avail)
                v_mov.push_back(item);

            return make_tuple(v_mov, 0.0);
        }

        if (state.cur_ply == 1)
            WriteToPipe(string("play W ") + loc_to_str(state.lst_mov.first, state.lst_mov.second));
        else
            WriteToPipe(string("play B ") + loc_to_str(state.lst_mov.first, state.lst_mov.second));
        Sleep(10);

        if (state.cur_ply == 1)
            WriteToPipe(string("lz-analyze B interval 4 maxmoves ") + tops);
        else
            WriteToPipe(string("lz-analyze W interval 4 maxmoves ") + tops);
        Sleep(60);
        WriteToPipe("\n");
        Sleep(20);

        auto result = ReadFromPipe();
        
        int num = std::stoi(tops), i = 1;

        auto pos = result.find("winrate");

        if (pos == string::npos)
        {
            end_process();
            init();
            int i = 1;
            for (auto item : state.moves)
            {
                if (i % 2 == 1)
                    WriteToPipe(string("play B ") + loc_to_str(item.first, item.second));
                else
                    WriteToPipe(string("play W ") + loc_to_str(item.first, item.second));
                Sleep(5);
                i++;
            }
            Sleep(20);
            
            if (state.cur_ply == 1)
                WriteToPipe(string("lz-analyze B interval 16 maxmoves ") + tops);
            else
                WriteToPipe(string("lz-analyze W interval 16 maxmoves ") + tops);
            Sleep(240);
            WriteToPipe("\n");
            Sleep(40);

            result = ReadFromPipe();
        }

        pos = result.find("move");

        while (pos != string::npos && i <= num)
        {
            double v = str_to_val(result.substr(result.find("winrate", pos + 1) + 8, 6));
            if (/*v >= -0.98 || i == */1)
                v_mov.push_back(str_to_mov(result.substr(pos + 5, 3), state));
            pos = result.find("move", pos + 1);
            i++;
        }

        double val = str_to_val(result.substr(result.find("winrate") + 8, 6));
        return make_tuple(v_mov, val);
    }

    auto get_move_single(Board state, string tops)
    {
        auto tup = get_move_list(state, tops);
        auto v_mov = get<0>(tup);
        auto val = get<1>(tup);
        mt19937 gen(random_device{}());

        if (state.lst_mov.first == 0)
        {
            state.resize();

            auto mov = v_mov[gen() % state.avail.size()];
            return make_tuple(mov, mov, 0.0);
        }

        pair<short, short> mov;
        if (gen() % 100 >= 50)
            mov = v_mov[gen() % v_mov.size()];
        else
            mov = v_mov[0];
        return make_tuple(v_mov[0], mov, val);
    }

    auto get_move(Board state)
    {
        if (state.lst_mov.first == 0)
        {
            state.resize();
            vector <pair<short, short>> v_mov;

            for (auto item : state.avail)
                v_mov.push_back(item);

            mt19937 gen(random_device{}());
            return v_mov[gen() % state.avail.size()];
        }

        WriteToPipe("clear_board");
        for (short i = 1; i <= 15; i++)
            for (short j = 1; j <= 15; j++)
            {
                if (state.plate[i][j] == 1)
                    WriteToPipe(string("play B ") + loc_to_str(i, j));
                else if (state.plate[i][j] == -1)
                    WriteToPipe(string("play W ") + loc_to_str(i, j));
                else
                    continue;
                Sleep(4);
            }
        if (state.cur_ply == 1)
            WriteToPipe(string("lz-analyze B interval 4 maxmoves 1"));
        else
            WriteToPipe(string("lz-analyze W interval 4 maxmoves 1"));
        Sleep(60);
        WriteToPipe("\n");
        Sleep(20);
        auto result = ReadFromPipe();

        pair<short, short> mov;
        mov = str_to_mov(result.substr(result.find("move") + 5, 3), state);
        return mov;
    }

    void end_process()
    {
        CloseHandle(g_hChildStd_OUT_Rd);
        CloseHandle(g_hChildStd_IN_Wr);
    }

    ~Katagomo()
    {
        CloseHandle(g_hChildStd_OUT_Rd);
        CloseHandle(g_hChildStd_IN_Wr);
    }

    void ErrorExit(PCTSTR lpszFunction)
    {
        LPVOID lpMsgBuf;
        DWORD dw = GetLastError();
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
        TCHAR szError[512];
        StringCchPrintf(szError, sizeof(szError) / sizeof(TCHAR), TEXT("%s failed with error %d: %s"), lpszFunction, dw, lpMsgBuf);

        MessageBox(NULL, szError, TEXT("Error"), MB_OK);
        LocalFree(lpMsgBuf);
        ExitProcess(1);
    }
};