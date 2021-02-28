#include <iostream>
#include <locale>
#include <codecvt>
#include <string>
#include <stdexcept>
#include <vector>
#include <fstream>
#include <sys/stat.h>
#include <windows.h>
#include <time.h>       /* time_t, struct tm, time, localtime, strftime */

using namespace std;

wstring utf8toUtf16(const string & str) // https://stackoverflow.com/questions/2573834/c-convert-string-or-char-to-wstring-or-wchar-t/8969776#8969776
{
    if (str.empty())
        return wstring();

    size_t charsNeeded = ::MultiByteToWideChar(CP_UTF8, 0,
                         str.data(), (int)str.size(), NULL, 0);
    if (charsNeeded == 0)
        throw runtime_error("Failed converting UTF-8 string to UTF-16");

    vector<wchar_t> buffer(charsNeeded);
    int charsConverted = ::MultiByteToWideChar(CP_UTF8, 0,
                         str.data(), (int)str.size(), &buffer[0], buffer.size());
    if (charsConverted == 0)
        throw runtime_error("Failed converting UTF-8 string to UTF-16");

    return wstring(&buffer[0], charsConverted);
}

wchar_t utf8toUtf16Char(const string & str) // https://stackoverflow.com/questions/2573834/c-convert-string-or-char-to-wstring-or-wchar-t/8969776#8969776
{
    wchar_t res=0;
    int charsConverted = ::MultiByteToWideChar(CP_UTF8, 0,
                         str.data(), (int)str.size(), &res, 1);
    if (charsConverted == 0)
        throw runtime_error("Failed converting UTF-8 string to UTF-16");
    return res;
}

string utf16toUtf8(const wstring & str)
{
    if (str.empty())
        return string();

    size_t charsNeeded = ::WideCharToMultiByte(CP_UTF8, 0,str.data(), (int)str.size(), NULL, 0, NULL, 0);
    if (charsNeeded == 0)
        throw runtime_error("Failed converting UTF-16 string to UTF-8");

    vector<char> buffer(charsNeeded);
    int charsConverted = ::WideCharToMultiByte(CP_UTF8, 0,
                         str.data(), (int)str.size(), &buffer[0], buffer.size(), NULL, 0);
    if (charsConverted == 0)
        throw runtime_error("Failed converting UTF-16 string to UTF-8");

    return string(&buffer[0], charsConverted);
}



class CFind_Event
{
public:
    int EndOfFile;
    int BegOfFile;

    CFind_Event()
    {
        Init();
    }

    void Init(void)
    {
        EndOfFile = 0;
        BegOfFile = 0;
    }
};

class CTextTorroise
{
public:
    wstring text;
    long long cursor;
    long long cursor_beg_sel;
    long long cursor_end_sel;
    long long pos_for_find;
    long long pos_find_result_atbegin;
    long long pos_find_result_atend;
    CFind_Event find_event;
    wstring find_text;
    long find_direction_forward;

    void Init(void)
    {
        text = L"";
        cursor = 0;
        cursor_beg_sel = 0;
        cursor_end_sel = 0;
        pos_for_find = 0;
        find_event.Init();
        find_text = L"";
        find_direction_forward = 1;
    }
    CTextTorroise()
    {
        Init();
    }

    long LoadFromFile(wstring FileName)
    {
        static char* buff = NULL;
        Init(); // а конструктор нельзя тут вручную вызывать! он создаёт скрытый объект. лучше вызывать метод\функцию.
        try
        {
            //std::wstring FileName2 = FileName;
            //FILE* fin = _wfopen(FileName.c_str(),L"rb"); // https://stackoverflow.com/questions/29187070/reading-from-a-binary-file-in-c/29187280
            FILE* fin = _wfopen(FileName.c_str(),L"rb"); // https://stackoverflow.com/questions/29187070/reading-from-a-binary-file-in-c/29187280
            //fseek(fin, 0L, SEEK_END);
            //long size2 = ftell(fin);
            //fseek(fin, 0L, SEEK_SET);
            long long size2=0;
            //GetFileSizeEx(fin,(PLARGE_INTEGER)&size2);
            struct __stat64 st;
            if (_wstat64(FileName.c_str(),&st)==0)
                size2 = st.st_size;
            if (size2 <= 0) return 0;

            if (buff != NULL) delete buff;
            buff = new char[size2];

            size_t len = fread(buff, 1, size2, fin);

            if (len != size2) return 0;

            // copies all data into buffer
            //std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(fin), {});

            this->text = utf8toUtf16(buff);
            //TFileStream *fin = new TFileStream(FileName, fmOpenRead);
            //char* buff = new char[fin->Size];
            //int len = fin->Read(buff, fin->Size);
            //this->text = UTF8ToWideString(buff);
            //fin->Free();
            fclose(fin);
            return 1;
        }
        catch (...)
        {
            return 5;
        }
    }

    long find(wstring substr)
    {

        if (find_direction_forward)
        {
            return find_forward(substr);
        }
        else
        {
            return find_backward(substr);
        }
    }

    long find_forward(wstring substr)
    {
        long matches_index = 0;
        long long pos1 = pos_for_find;
        do
        {
            if (pos1 > text.length())
            {
                find_event.EndOfFile = 1;
                return 0;
            }
            if (substr.operator[](matches_index) == text.operator[](pos1))
            {
                //wcout << substr.operator[](matches_index);
                //wcout << text.operator[](pos1);
                char_detector(text.operator[](pos1));
                matches_index++;
                pos1++;
                if ((size_t)matches_index >= substr.length())
                {
                    // found
                    pos_find_result_atend = pos1;// after
                    pos_for_find = pos1;
                    pos_find_result_atbegin = (pos1 - matches_index)-1; // before
                    if (pos_find_result_atbegin<0) pos_find_result_atbegin = 0;
                    return 1;
                }
            }
            else
            {
                //wcout << 'f';
                pos1 = (pos1 - matches_index) + 1;
                matches_index = 0;
            }
        }
        while (1);
        return 1;
    }

    long find_backward(wstring substr)
    {
        long matches_index = substr.length()-1;//0
        long long pos1 = pos_for_find;
        do
        {
            if (pos1 < 0 ) // > text.length()
            {
                find_event.BegOfFile = 1;
                return 0;
            }
            if (substr.operator[](matches_index) == text.operator[](pos1))
            {
                //wcout << substr.operator[](matches_index);
                //wcout << text.operator[](pos1);
                char_detector(text.operator[](pos1));
                matches_index--;
                pos1--;
                if (matches_index < 0) // >= substr.length())
                {
                    // found
                    pos_find_result_atbegin = pos1; // before word
                    pos_find_result_atend = (pos1 + substr.length()); // after word
                    pos_for_find = pos_find_result_atend;
                    return 1;
                }
            }
            else
            {
                //wcout << 'f';
                pos1 = (pos1 + substr.length()-1 - matches_index) - 1;
                matches_index = substr.length()-1;//0;
            }
        }
        while (1);
        return 1;
    }
    wchar_t char_detector_char;
    long char_detector_cnt;

    wstring text_inside(wstring& find1,wstring& find2)
    {
        //wstring find1 = utf8toUtf16(R"EOF("page_count":)EOF" );
        this->find_direction_forward = 1;
        this->find(find1);
        long selbeg = this->pos_find_result_atend;
        //wstring find2 = utf8toUtf16(R"EOF(,)EOF" );
        this->find_direction_forward = 1;
        this->find(find2);
        long selend = this->pos_find_result_atbegin;

        wstring sel_text = this->text.substr(selbeg,selend-(selbeg)+1);
        return sel_text;
    }



private:
    void char_detector(wchar_t ch)
    {
        if (ch == char_detector_char)
            char_detector_cnt++;
    }
};

CTextTorroise ctt;


void ExtractFileDirMod(wstring& s)
{
    long i= 0;
    while (s[i] != 0)
    {
        i++;
    }

    int len = i;
    for (int j = len-1; j>=0; j--)
    {
        if (s[j] != '\\')
            //s[j] = '\x00';
            s.pop_back();
        else
            break;
    }
}


void exec(wstring Prog,wstring Param,wstring Dir)
{
    SHELLEXECUTEINFO ShExecInfo = {0};
    ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    ShExecInfo.hwnd = NULL;
    ShExecInfo.lpVerb = NULL;
    ShExecInfo.lpFile = Prog.c_str();
    ShExecInfo.lpParameters = Param.c_str();
    ShExecInfo.lpDirectory = Dir.c_str();
    ShExecInfo.nShow = SW_HIDE;//SW_SHOW;
    ShExecInfo.hInstApp = NULL;
    ShellExecuteEx(&ShExecInfo);
    WaitForSingleObject(ShExecInfo.hProcess, INFINITE);
    CloseHandle(ShExecInfo.hProcess);
}

bool erase_substring(wstring& str, wstring& find_it)
{
    size_t start_pos = str.find(find_it);
    if(start_pos == std::string::npos)
        return false;
    str.erase(start_pos, find_it.length());
    return true;
}




//int main()
int wmain(int argc, wchar_t *argv[])
{
    wstring apibase =
        L"http://api2.goodgame.ru";

    long long ind_pages = 0;
    char ch;
    setlocale(LC_ALL, "Russian");
    wcout << L"программа скачки Смайлов. " << endl;
    wcout << L"1 для скачки списка " << endl;
    wcout << L"2 для скачки картинок " << endl;
    wcout << L"3 для генерации си-таблицы " << endl;

    if (argc>1)
        ch = argv[1][0];
    else
        cin >> ch;

    int skip_curl = 0;
    int lock_ind = 0;
    if (0==1)
    {
        //test
        ind_pages=27;
        //skip_curl=1;
        lock_ind=1;
        apibase = L"file:///";
    }
    //ch = '1';
    //wcout << ch << endl;

    wstring virus = L"\\";

    ind_pages = 1;


    //wstring DirEXE = argv[0];//ExtractFileDir(Application->ExeName);
    // https://stackoverflow.com/questions/143174/how-do-i-get-the-directory-that-a-program-is-running-from
    wstring DirEXE=L"";
    {
    wchar_t pBuf[2560];
    size_t len = 2560;
    int bytes = GetModuleFileName(NULL, pBuf, len);
    if (bytes)
    DirEXE = pBuf;
    }

    ExtractFileDirMod(DirEXE);

    wstring str_date=L"";
    {
        // https://www.cplusplus.com/reference/ctime/strftime/
        static wchar_t wchar_date[80]= {0};
        time_t rawtime;
        struct tm * timeinfo=NULL;

        time (&rawtime);
        timeinfo = localtime (&rawtime);

// lol %F do not works https://www.cplusplus.com/reference/ctime/strftime/
        //strftime (wchar_date,80,"%F",timeinfo);
        wcsftime(wchar_date,80,L"%Y%m%d",timeinfo);
        str_date = wchar_date;
    }

    wcout << L"сегодняшняя подпапка -- ";
    wcout << str_date;
    wcout << L"\n";
    wstring Dir = DirEXE+str_date;

// новую папку
    exec(L"cmd",L"/C mkdir \"" + Dir + L"\"",L"");
// операционка возможно захочет "просраться"
    Sleep(100);
// новые подпапки
    wstring Dir_img = Dir+L"\\img";
    wstring Dir_big = Dir+L"\\big";
    wstring Dir_gif = Dir+L"\\gif";
    exec(L"cmd",L"/C mkdir \"" + Dir_img + L"\"",L"");
    exec(L"cmd",L"/C mkdir \"" + Dir_big + L"\"",L"");
    exec(L"cmd",L"/C mkdir \"" + Dir_gif + L"\"",L"");

    wstring Prog = DirEXE+L"curl.exe";

    Sleep(100);

#define curl_cooldown 200

    if (ch=='1')
        do
        {
            wstring nn = std::to_wstring(ind_pages);
            wcout << nn << endl;
            wstring Param_for_curl = L"-o json_smile_list_" + nn + L".json " + apibase + L"/smiles?page="+ nn;
            //wstring Dir2 = Dir.SubString(1, Dir.Length() - 2);

            if (skip_curl==0)
                exec(Prog,Param_for_curl,Dir);
            Sleep(curl_cooldown);

            if (ctt.LoadFromFile(Dir+L"\\"+L"json_smile_list_" + nn + L".json"))
            {

                static long total_pages = 0;

                if (total_pages==0)
                {
                    wstring find1 = utf8toUtf16(R"EOF("page_count":)EOF" );
                    wstring find2 = utf8toUtf16(R"EOF(,)EOF" );
                    wstring sel_text = ctt.text_inside(find1,find2);
                    if (ctt.find_event.EndOfFile)
                    {
                        wcout << L"приехали 3" << endl;
                        cin >> ch;
                        return 1;
                    }

                    wcout << L"всего страниц " << sel_text << endl;
                    total_pages = stoi(sel_text);
                    //return 8;
                }

                if (lock_ind==0)
                    ind_pages++;

                if (ind_pages > total_pages) break;

            }
            else
            {
                return 2;
            }



        }
        while (1);

    wstring ci_table =L"";
    if ((ch=='3'))
    {
        skip_curl = 1;

        ci_table.reserve(4*1048576); // reserve 1 MB // https://stackoverflow.com/questions/3303527/how-to-pre-allocate-memory-for-a-stdstring-object
        ci_table =L"";
        //if (ch=='3') ci_table = ci_table + L"#define smile_key_No_"++"\n";

#define TABBB(NAMEEE,VALUEEE) if (ch=='3') { wstring add = L""; add = add+L"#define smile_"+NAMEEE+L"_No_"+s_smile_number+L"    \""+ VALUEEE +L"\"\n"; ci_table.append( add.c_str() ); }

        //if (ch=='3') ci_table = ci_table + L"#define smiles_total_"++"\n";
#define TOTALLL { wstring sss_smile_number = std::to_wstring(smile_number); wstring add = L""; add = add + L"#define smiles_total    "+sss_smile_number+L"\n"; if (ch=='3') ci_table.append(  add.c_str() ); }


    }

    // cont if (ind_pages >= total_pages) break;
    if (
        (ch=='2') // скачки картинок
        || (ch=='3')
    )
    {

        Sleep(100);

        ind_pages = 0;
        long long smile_number = 0;
        do
        {
            ind_pages++;
            wstring nn = std::to_wstring(ind_pages);
            if (ctt.LoadFromFile(Dir+L"\\"+L"json_smile_list_" + nn + L".json"))
            {
                wstring findbeg = utf8toUtf16(R"EOF(,"_embedded":{)EOF" );
                ctt.find_direction_forward = 1;
                ctt.find(findbeg);

                wcout << L"стр." << nn << "\n";

                do
                {
                    smile_number++;
                    wstring s_smile_number = std::to_wstring(smile_number);
                    wstring name_key;
                    {
                        wstring find1 = utf8toUtf16(R"EOF("key":")EOF" );
                                        wstring find2 = utf8toUtf16(R"EOF(",")EOF" );
                                        wstring sel_text = ctt.text_inside(find1,find2);
                                        if (ctt.find_event.EndOfFile)
                                        break;
                                        name_key = sel_text;
                                    }
                                        wcout << L"    смайл No." << s_smile_number << "=" << name_key << "\n";

                                                //if (ch=='3') ci_table = ci_table + L"#define smile_key_No_"+s_smile_number+L" = "+ name_key +L"\n";
                                                        TABBB(L"key",name_key);

                                                        name_key = name_key+L"__"+s_smile_number;

                                                        TABBB(L"key_with_No",name_key);

                                                        {
                                wstring find1 = utf8toUtf16(R"EOF("img":")EOF" );
                        wstring find2 = utf8toUtf16(R"EOF(",)EOF" );
                                        wstring sel_text = ctt.text_inside(find1,find2);
                                        if (ctt.find_event.EndOfFile)
                                        break;
                                        while (erase_substring(sel_text,virus)) {};

                                        wstring Param_for_curl = L"-o " + name_key + L"_small.png " + L"-L "+sel_text;
                                        if (skip_curl==0)
                                        {
                                        exec(Prog,Param_for_curl,Dir_img);
                                        Sleep(curl_cooldown);
                                    }

                                        TABBB(L"small_png_filename",name_key + L"_small.png");
                                        TABBB(L"small_png_url",sel_text);


                                    }

                                        {
                                        wstring find1 = utf8toUtf16(R"EOF(big":")EOF" );
                                        wstring find2 = utf8toUtf16(R"EOF(",)EOF" );
                        wstring sel_text = ctt.text_inside(find1,find2);
                        if (ctt.find_event.EndOfFile)
                            break;
                        while (erase_substring(sel_text,virus)) {};

                        wstring Param_for_curl = L"-o " + name_key + L".png " + L"-L "+sel_text;
                        if (skip_curl==0)
                        {
                            exec(Prog,Param_for_curl,Dir_big);
                            Sleep(curl_cooldown);
                        }

                        TABBB(L"png_filename",name_key + L".png");
                        TABBB(L"png_url",sel_text);

                    }

                    {
                        wstring find1 = utf8toUtf16(R"EOF("gif":")EOF" );
                                        wstring find2 = utf8toUtf16(R"EOF("})EOF" );
                        wstring sel_text = ctt.text_inside(find1,find2);
                        if (ctt.find_event.EndOfFile)
                            break;
                        while (erase_substring(sel_text,virus)) {};

                        wstring Param_for_curl = L"-o " + name_key + L".gif " + L"-L "+sel_text;
                        if (skip_curl==0)
                        {
                            exec(Prog,Param_for_curl,Dir_gif);
                            Sleep(curl_cooldown);
                        }
                        TABBB(L"gif_filename",name_key + L".gif");
                        TABBB(L"gif_url",sel_text);
                    }

                }
                while (1);

            } // if
            else
                break;
        }
        while(1);   // do

        TOTALLL;

        if (ch=='3')
        {
            // https://goodgame.ru
            // https://goodgame.ru/html/user-agreement.html
            ci_table = ci_table + L"// https://goodgame.ru/html/user-agreement.html \n";
            ci_table = ci_table + L"// goodgame.ru \n";

            string test = utf16toUtf8(ci_table);
            wstring FileName = Dir+L"\\"+L"smile_table.h";
            FILE* fout = _wfopen(FileName.c_str(),L"wb");
            fwrite(test.c_str(), 1, test.size(), fout);
            fclose(fout);
        }



    } // if ch


    return 0;
}

