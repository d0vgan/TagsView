#include "TagsCommon.h"
#include <vector>

namespace TagsCommon
{
    bool isFileExist(LPCTSTR pszFilePath)
    {
        if ( pszFilePath && pszFilePath[0] )
        {
            if ( ::GetFileAttributes(pszFilePath) != INVALID_FILE_ATTRIBUTES )
                return true;
        }
        return false;
    }

    LPCTSTR getFileName(LPCTSTR pszFilePathName)
    {
        if ( pszFilePathName && *pszFilePathName )
        {
            int n = lstrlen(pszFilePathName);
            while ( --n >= 0 )
            {
                if ( (pszFilePathName[n] == _T('\\')) || (pszFilePathName[n] == _T('/')) )
                {
                    break;
                }
            }

            if ( n >= 0 )
            {
                ++n;
                pszFilePathName += n;
            }
        }
        return pszFilePathName;
    }

    LPCTSTR getFileName(const t_string& filePathName)
    {
        t_string::size_type n = 0;
        if ( !filePathName.empty() )
        {
            n = filePathName.find_last_of(_T("\\/"));
            if ( n != t_string::npos )
                ++n;
            else
                n = 0;
        }
        return (filePathName.c_str() + n);
    }

    LPCTSTR getFileName(const tTagData* pTag)
    {
        if ( !pTag->hasFilePath() )
            return _T("");

        const t_string& filePath = *pTag->pFilePath;
        if ( filePath.length() <= pTag->nFileDirLen )
            return getFileName(filePath);

        size_t n = pTag->nFileDirLen;
        if ( filePath[n] == _T('\\') || filePath[n] == _T('/') )  ++n;
        return (filePath.c_str() + n);
    }

    LPCTSTR getFileExt(LPCTSTR pszFilePathName)
    {
        const TCHAR* pszExt = pszFilePathName + lstrlen(pszFilePathName);
        while ( --pszExt >= pszFilePathName )
        {
            if ( *pszExt == _T('.') )
                return pszExt;

            if ( *pszExt == _T('\\') || *pszExt == _T('/') )
                break;
        }
        return _T("");
    }

    t_string getFileDirectory(const t_string& filePathName)
    {
        t_string::size_type n = filePathName.find_last_of(_T("\\/"));
        return ( (n != t_string::npos) ? t_string(filePathName.c_str(), n + 1) : t_string() );
    }


    bool setClipboardText(const t_string& text, HWND hWndOwner)
    {
        bool bSucceeded = false;

        if ( ::OpenClipboard(hWndOwner) )
        {
            HGLOBAL hTextMem = ::GlobalAlloc( GMEM_MOVEABLE, (text.length() + 1)*sizeof(TCHAR) );
            if ( hTextMem != NULL )
            {
                LPTSTR pszText = (LPTSTR) ::GlobalLock(hTextMem);
                if ( pszText != NULL )
                {
                    lstrcpy(pszText, text.c_str());
                    ::GlobalUnlock(hTextMem);

                    ::EmptyClipboard();

#ifdef UNICODE
                    const UINT uClipboardFormat = CF_UNICODETEXT;
#else
                    const UINT uClipboardFormat = CF_TEXT;
#endif
                    if ( ::SetClipboardData(uClipboardFormat, hTextMem) != NULL )
                        bSucceeded = true;
                }
            }
            ::CloseClipboard();
        }

        return bSucceeded;
    }

    t_string getCtagsLangFamily(const t_string& filePath)
    {
        struct tCtagsLangFamily
        {
            const TCHAR* cszLangName;
            std::vector<const TCHAR*> arrLangFiles;
        };

        // Note: this list can be obtained by running `ctags --list-maps`
        static const tCtagsLangFamily languages[] = {
            { _T("Abaqus"),          { _T(".inp") } },
            { _T("Abc"),             { _T(".abc") } },
            { _T("Ada"),             { _T(".adb"), _T(".ads"), _T(".ada") } },
            { _T("Ant"),             { _T("build.xml"), _T(".build.xml"), _T(".ant") } },
            { _T("Asciidoc"),        { _T(".asc"), _T(".adoc"), _T(".asciidoc") } },
            { _T("Asm"),             { _T(".a51"), _T(".29k"), _T(".x86"), _T(".asm"), _T(".s") } },
            { _T("Asp"),             { _T(".asp"), _T(".asa") } },
            { _T("Autoconf"),        { _T("configure.in"), _T(".in"), _T(".ac") } },
            { _T("AutoIt"),          { _T(".au3") } },
            { _T("Automake"),        { _T(".am") } },
            { _T("Awk"),             { _T(".awk"), _T(".gawk"), _T(".mawk") } },
            { _T("Basic"),           { _T(".bas"), _T(".bi"), _T(".bm"), _T(".bb"), _T(".pb") } },
            { _T("BETA"),            { _T(".bet") } },
            { _T("BibTeX"),          { _T(".bib") } },
            { _T("Clojure"),         { _T(".clj"), _T(".cljs"), _T(".cljc") } },
            { _T("CMake"),           { _T("CMakeLists.txt"), _T(".cmake") } },
            { _T("C"),               { _T(".c") } },
            { _T("C++"),             { _T(".c++"), _T(".cc"), _T(".cp"), _T(".cpp"), _T(".cxx"), _T(".h"), _T(".h++"), _T(".hh"), _T(".hp"), _T(".hpp"), _T(".hxx"), _T(".inl") } },
            { _T("CSS"),             { _T(".css") } },
            { _T("C#"),              { _T(".cs") } },
            { _T("Ctags"),           { _T(".ctags") } },
            { _T("Cobol"),           { _T(".cbl"), _T(".cob") } },
            { _T("CUDA"),            { _T(".cu"), _T(".cuh") } },
            { _T("D"),               { _T(".d"), _T(".di") } },
            { _T("Diff"),            { _T(".diff"), _T(".patch") } },
            { _T("DTD"),             { _T(".dtd"), _T(".mod") } },
            { _T("DTS"),             { _T(".dts"), _T(".dtsi") } },
            { _T("DosBatch"),        { _T(".bat"), _T(".cmd") } },
            { _T("Eiffel"),          { _T(".e") } },
            { _T("Elixir"),          { _T(".ex"), _T(".exs") } },
            { _T("EmacsLisp"),       { _T(".el") } },
            { _T("Erlang"),          { _T(".erl"), _T(".hrl") } },
            { _T("Falcon"),          { _T(".fal"), _T(".ftd") } },
            { _T("Flex"),            { _T(".as"), _T(".mxml") } },
            { _T("Fortran"),         { _T(".f"), _T(".for"), _T(".ftn"), _T(".f77"), _T(".f90"), _T(".f95"), _T(".f03"), _T(".f08"), _T(".f15") } },
            { _T("Fypp"),            { _T(".fy") } },
            { _T("Gdbinit"),         { _T(".gdbinit"), _T(".gdb") } },
            { _T("GDScript"),        { _T(".gd") } },
            { _T("GemSpec"),         { _T(".gemspec") } },
            { _T("Go"),              { _T(".go") } },
            { _T("Haskell"),         { _T(".hs") } },
            { _T("Haxe"),            { _T(".hx") } },
            { _T("HTML"),            { _T(".htm"), _T(".html") } },
            { _T("Iniconf"),         { _T(".ini"), _T(".conf") } },
            { _T("Inko"),            { _T(".inko") } },
            { _T("ITcl"),            { _T(".itcl") } },
            { _T("Java"),            { _T(".java") } },
            { _T("JavaProperties"),  { _T(".properties") } },
            { _T("JavaScript"),      { _T(".js"), _T(".jsx"), _T(".mjs") } },
            { _T("JSON"),            { _T(".json") } },
            { _T("Julia"),           { _T(".jl") } },
            { _T("LdScript"),        { _T(".lds.s"), _T("ld.script"), _T(".lds"), _T(".scr"), _T(".ld"), _T(".ldi") } },
            { _T("LEX"),             { _T(".lex"), _T(".l") } },
            { _T("Lisp"),            { _T(".cl"), _T(".clisp"), _T(".l"), _T(".lisp"), _T(".lsp") } },
            { _T("LiterateHaskell"), { _T(".lhs") } },
            { _T("Lua"),             { _T(".lua") } },
            { _T("M4"),              { _T(".m4"), _T(".spt") } },
            { _T("Man"),             { _T(".1"), _T(".2"), _T(".3"), _T(".4"), _T(".5"), _T(".6"), _T(".7"), _T(".8"), _T(".9"), _T(".3pm"), _T(".3stap"), _T(".7stap") } },
            { _T("Make"),            { _T("makefile"), _T("GNUmakefile"), _T(".mak"), _T(".mk") } },
            { _T("Markdown"),        { _T(".md"), _T(".markdown") } },
            { _T("MatLab"),          { _T(".m") } },
            { _T("Meson"),           { _T("meson.build") } },
            { _T("MesonOptions"),    { _T("meson_options.txt") } },
            { _T("Myrddin"),         { _T(".myr") } },
            { _T("NSIS"),            { _T(".nsi"), _T(".nsh") } },
            { _T("ObjectiveC"),      { _T(".mm"), _T(".m"), _T(".h") } },
            { _T("OCaml"),           { _T(".ml"), _T(".mli"), _T(".aug") } },
            { _T("Org"),             { _T(".org") } },
            { _T("Passwd"),          { _T("passwd") } },
            { _T("Pascal"),          { _T(".p"), _T(".pas") } },
            { _T("Perl"),            { _T(".pl"), _T(".pm"), _T(".ph"), _T(".plx"), _T(".perl") } },
            { _T("Perl6"),           { _T(".p6"), _T(".pm6"), _T(".pm"), _T(".pl6") } },
            { _T("PHP"),             { _T(".php"), _T(".php3"), _T(".php4"), _T(".php5"), _T(".php7"), _T(".phtml") } },
            { _T("Pod"),             { _T(".pod") } },
            { _T("PowerShell"),      { _T(".ps1"), _T(".psm1") } },
            { _T("Protobuf"),        { _T(".proto") } },
            { _T("PuppetManifest"),  { _T(".pp") } },
            { _T("Python"),          { _T(".py"), _T(".pyx"), _T(".pxd"), _T(".pxi"), _T(".scons"), _T(".wsgi") } },
            { _T("QemuHX"),          { _T(".hx") } },
            { _T("RMarkdown"),       { _T(".rmd") } },
            { _T("R"),               { _T(".r"), _T(".s"), _T(".q") } },
            { _T("Rake"),            { _T("Rakefile"), _T(".rake") } },
            { _T("REXX"),            { _T(".cmd"), _T(".rexx"), _T(".rx") } },
            { _T("Robot"),           { _T(".robot") } },
            { _T("RpmSpec"),         { _T(".spec") } },
            { _T("ReStructuredText"),{ _T(".rest"), _T(".rst") } },
            { _T("Ruby"),            { _T(".rb"), _T(".ruby") } },
            { _T("Rust"),            { _T(".rs") } },
            { _T("Scheme"),          { _T(".sch"), _T(".scheme"), _T(".scm"), _T(".sm"), _T(".rkt") } },
            { _T("SCSS"),            { _T(".scss") } },
            { _T("Sh"),              { _T(".sh"), _T(".bsh"), _T(".bash"), _T(".ksh"), _T(".zsh"), _T(".ash") } },
            { _T("SLang"),           { _T(".sl") } },
            { _T("SML"),             { _T(".sml"), _T(".sig") } },
            { _T("SQL"),             { _T(".sql") } },
            { _T("SystemdUnit"),     { _T(".service"), _T(".socket"), _T(".device"), _T(".mount"), _T(".automount"), _T(".swap"), _T(".target"), _T(".path"), _T(".timer"), _T(".snapshot"), _T(".slice") } },
            { _T("SystemTap"),       { _T(".stp"), _T(".stpm") } },
            { _T("Tcl"),             { _T(".tcl"), _T(".tk"), _T(".wish"), _T(".exp") } },
            { _T("Tex"),             { _T(".tex") } },
            { _T("TTCN"),            { _T(".ttcn"), _T(".ttcn3") } },
            { _T("Txt2tags"),        { _T(".t2t") } },
            { _T("TypeScript"),      { _T(".ts") } },
            { _T("Vera"),            { _T(".vr"), _T(".vri"), _T(".vrh") } },
            { _T("Verilog"),         { _T(".v") } },
            { _T("SystemVerilog"),   { _T(".sv"), _T(".svh"), _T(".svi") } },
            { _T("VHDL"),            { _T(".vhdl"), _T(".vhd") } },
            { _T("Vim"),             { _T("vimrc"), _T(".vimrc"), _T("_vimrc"), _T("gvimrc"), _T(".gvimrc"), _T("_gvimrc"), _T(".vim"), _T(".vba") } },
            { _T("WindRes"),         { _T(".rc") } },
            { _T("YACC"),            { _T(".y") } },
            { _T("YumRepo"),         { _T(".repo") } },
            { _T("Zephir"),          { _T(".zep") } },
            { _T("Glade"),           { _T(".glade") } },
            { _T("Maven2"),          { _T("pom.xml"), _T(".pom"), _T(".xml") } },
            { _T("PlistXML"),        { _T(".plist") } },
            { _T("RelaxNG"),         { _T(".rng") } },
            { _T("SVG"),             { _T(".svg") } },
            { _T("XML"),             { _T(".xml") } },
            { _T("XSLT"),            { _T(".xsl"), _T(".xslt") } },
            { _T("Yaml"),            { _T(".yml"), _T(".yaml") } },
            { _T("OpenAPI"),         { _T("openapi.yaml") } },
            { _T("Varlink"),         { _T(".varlink") } },
            { _T("Kotlin"),          { _T(".kt"), _T(".kts") } },
            { _T("Thrift"),          { _T(".thrift") } },
            { _T("Elm"),             { _T(".elm") } },
            { _T("RDoc"),            { _T(".rdoc") } }
        };

        auto ends_with = [](const t_string& fileName, const TCHAR* cszEnd) -> bool
        {
            size_t nEndLen = lstrlen(cszEnd);
            if ( nEndLen <= fileName.length() )
            {
                if ( lstrcmpi(fileName.c_str() + fileName.length() - nEndLen, cszEnd) == 0 )
                    return true;
            }
            return false;
        };

        const TCHAR* pszExt = getFileExt(filePath.c_str());
        const TCHAR* pszFileName = getFileName(filePath);
        for ( const tCtagsLangFamily& lang : languages )
        {
            for ( const TCHAR* pattern : lang.arrLangFiles )
            {
                if ( pattern[0] == _T('.') )
                {
                    if ( *pszExt )
                    {
                        if ( ends_with(filePath, pattern) )
                        {
                            if ( lstrcmp(lang.cszLangName, _T("C")) == 0 || lstrcmp(lang.cszLangName, _T("C++")) == 0 )
                            {
                                return t_string(_T("C,C++"));
                            }
                            return t_string(lang.cszLangName);
                        }
                    }
                }
                else
                {
                    if ( lstrcmpi(pszFileName, pattern) == 0 )
                    {
                        return t_string(lang.cszLangName);
                    }
                }
            }
        }

        return t_string();
    }

    std::list<t_string> getRelatedSourceFiles(const t_string& fileName)
    {
        // C/C++ source files
        static const TCHAR* arrSourceCCpp[] = {
            _T(".c"),
            _T(".cc"),
            _T(".cpp"),
            _T(".cxx"),
            nullptr
        };

        // C/C++ header files
        static const TCHAR* arrHeaderCCpp[] = {
            _T(".h"),
            _T(".hh"),
            _T(".hpp"),
            _T(".hxx"),
            nullptr
        };

        auto processFileExtensions = [](const t_string& fileName,
                                        const TCHAR* arrExts1[],
                                        const TCHAR* arrExts2[],
                                        std::list<t_string>& relatedFiles) -> bool
        {
            int nMatchIndex = -1;

            const TCHAR* pszExt = getFileExt(fileName.c_str());
            if ( *pszExt )
            {
                for ( int i = 0; arrExts1[i] != nullptr && nMatchIndex == -1; ++i )
                {
                    if ( lstrcmpi(pszExt, arrExts1[i]) == 0 )
                        nMatchIndex = i;
                }

                if ( nMatchIndex != -1 )
                {
                    t_string relatedFile = fileName;
                    for ( int i = 0; arrExts2[i] != nullptr; ++i )
                    {
                        relatedFile.erase(relatedFile.rfind(_T('.')));
                        relatedFile += arrExts2[i];
                        if ( isFileExist(relatedFile.c_str()) )
                        {
                            relatedFiles.push_back(relatedFile);
                        }
                    }
                }
            }

            return (nMatchIndex != -1);
        };

        std::list<t_string> relatedFiles;

        if ( processFileExtensions(fileName, arrSourceCCpp, arrHeaderCCpp, relatedFiles) )
            return relatedFiles;

        if ( processFileExtensions(fileName, arrHeaderCCpp, arrSourceCCpp, relatedFiles) )
            return relatedFiles;

        return std::list<t_string>();
    }
}
