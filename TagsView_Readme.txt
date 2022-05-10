TagsView plugin for Notepad++ and AkelPad

OVERVIEW

  TagsView uses ctags to parse a source file and to show the result in a docked window.
  The ctags project is here:
    https://github.com/universal-ctags/ctags
    https://github.com/universal-ctags/ctags-win32

  TagsView provides the same user interface for Notepad++ and AkelPad text editors.
  Notepad++ is here:
    https://notepad-plus-plus.org/
    https://github.com/notepad-plus-plus/notepad-plus-plus
  AkelPad is here:
    http://akelpad.sourceforge.net/en/index.php
    https://sourceforge.net/projects/akelpad/


INSTALLATION

  Notepad++
    1.  Create a folder Notepad++\plugins\TagsView
    2a. For 32-bit Notepad++, extract NppTags\32bit\TagsView.dll to that folder
    2b. For 64-bit Notepad++, extract NppTags\64bit\TagsView.dll to that folder
    3.  Create a folder Notepad++\plugins\TagsView\TagsView
    4.  Extract all the files under Plugin\TagsView to that folder
    As the result, you'll have the following folder structure:

      Notepad++\plugins\TagsView
        TagsView.dll
        TagsView\
          ctags.1.html
          ctags.exe
          ctags.opt

  AkelPad
    1.  Go to the folder AkelPad\AkelFiles\Plugs
    2a. For 32-bit AkelPad, extract AkelTags\32bit\TagsView.dll to that folder
    2b. For 64-bit AkelPad, extract AkelTags\64bit\TagsView.dll to that folder
    3.  Create a folder AkelPad\AkelFiles\Plugs\TagsView
    4.  Extract all the files under Plugin\TagsView to that folder
    As the result, you'll have the following folder structure:

      AkelPad\AkelFiles\Plugs
        TagsView.dll
        TagsView\
          ctags.1.html
          ctags.exe
          ctags.opt


SETTINGS

  Parse on save (auto-parse file when it is saved)
  - TagsView automatically invokes ctags when you save the current file.

  Use Editor colors
  - TagsView applies the Editor's text and background colors to its user interface.

  Show tooltips
  - TagsView shows tooltips in its TreeView and ListView.

  ctags: output to stdout instead of temp file
  - TagsView reads ctags' output from stdout instead of a temporary file.
    This is slower than reading from a temp file. Moreover, ctags generates a temp file anyway.

  Scan similar files in a folder
  - TagsView instructs ctags to parse all the files with the similar extensions within the same folder.
    For example, it will parse all *.bat and *.cmd files at the same level as the opened .bat or .cmd file.
