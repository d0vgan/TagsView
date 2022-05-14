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

  Esc sets the focus to Editor
  - When Esc is pressed in TagsView, the focus goes to the Editor's editing window.
    (Note: you can always use Ctrl+BackSpace to clear the TagsView's filter).

  ctags: output to stdout instead of temp file
  - TagsView reads ctags' output from stdout instead of a temporary file.
    This is slower than reading from a temp file. Moreover, ctags generates a temp file anyway.

  Scan similar files in a folder
  - TagsView instructs ctags to parse all the files with the similar extensions within the same folder.
    For example, it will parse all *.bat and *.cmd files at the same level as the opened .bat or .cmd file.


MANUAL SETTINGS

  Notepad++

    [Ctags]
    SkipFileExts=.txt

      This setting specifies files extensions for which ctags will not be run.
      You can specify several extensions separated by spaces e.g.: .txt .diz .ion

    [Debug]
    DeleteTempInputFiles=1
    DeleteTempOutputFiles=1

      These settings specify when the temporary files are deleted. Possible values:
        0 - don't delete (for debug purpose)
        1 - delete once no more needed
        2 - preserve the last temp file until the next Parse() or exiting
      Temporary files are created under your %TEMP% folder.
      The temporary input files are created by TagsView for Unicode input files.
      The names of temporary input files start with "npp_inp_".
      The temporary output files are created by ctags.
      The names of temporary output files start with "npptags_".

  AkelPad

    [Options]
    Ctags\SkipFileExts=.txt

      This setting specifies files extensions for which ctags will not be run.
      You can specify several extensions separated by spaces e.g.: .txt .diz .ion

    Debug\DeleteTempInputFiles=1
    Debug\DeleteTempOutputFiles=1

      These settings specify when the temporary files are deleted. Possible values:
        0 - don't delete (for debug purpose)
        1 - delete once no more needed
        2 - preserve the last temp file until the next Parse() or exiting
      Temporary files are created under your %TEMP% folder.
      The temporary input files are created by TagsView for Unicode input files.
      The names of temporary input files start with "akel_inp_".
      The temporary output files are created by ctags.
      The names of temporary output files start with "akeltags_".
