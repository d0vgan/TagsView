#ifndef _TAGSVIEW_RESOURCE_H_FILE_
#define _TAGSVIEW_RESOURCE_H_FILE_

#define QUOTE(x) #x
#define STR(x) QUOTE(x)

#define PLUGIN_NAME        "TagsView"
#define PLUGIN_VERSION     0, 5, 0, 0
#define PLUGIN_COPYRIGHT   "(C) 2022, Vitaliy Dovgan"
#define PLUGIN_COMMENTS    "Glory to Ukraine! Glory to the heroes!"

#ifndef IDC_STATIC
#define IDC_STATIC (-1)
#endif

#define IDD_MAIN                                101
#define IDD_SETTINGS                            102

#define IDI_TAGSVIEW                            200
#define IDB_TAGSVIEW                            201
#define IDB_TOOLBAR                             203
#define IDB_EDITSPY                             204
#define IDB_CANCEL                              205
//#define IDC_ST_CAPTION                          1001
#define IDC_ED_FILTER                           1003
#define IDC_LV_TAGS                             1007
#define IDC_TV_TAGS                             1008
#define IDC_CH_OPT_PARSEONSAVE                  2001
#define IDC_CH_OPT_EDITORCOLORS                 2011
#define IDC_CH_OPT_SHOWTOOLTIPS                 2012
#define IDC_CH_OPT_NESTEDSCOPETREE              2013
#define IDC_CH_OPT_DBLCLICKTREE                 2014
#define IDC_CH_OPT_ESCFOCUSTOEDITOR             2015
#define IDC_CH_OPT_CTAGSSTDOUT                  2021
#define IDC_CH_OPT_SCANFOLDER                   2031
#define IDC_CH_OPT_SCANFOLDERRECURSIVELY        2032
#define IDR_TAGSVIEW_MENU                       3001
#define IDM_PREVPOS                             40000
#define IDM_NEXTPOS                             40001
#define IDM_SORTLINE                            40002
#define IDM_SORTNAME                            40003
#define IDM_VIEWLIST                            40004
#define IDM_VIEWTREE                            40005
#define IDM_PARSE                               40006
#define IDM_CLOSE                               40007
#define IDM_TREE_COPYITEMTOCLIPBOARD            40101
#define IDM_TREE_COPYITEMANDCHILDRENTOCLIPBOARD 40102
#define IDM_TREE_COPYALLITEMSTOCLIPBOARD        40103
#define IDM_TREE_EXPANDCHILDNODES               40111
#define IDM_TREE_COLLAPSECHILDNODES             40112
#define IDM_TREE_EXPANDALLNODES                 40113
#define IDM_TREE_COLLAPSEALLNODES               40114
#define IDM_LIST_COPYITEMTOCLIPBOARD            40201
#define IDM_LIST_COPYALLITEMSTOCLIPBOARD        40202
#define IDM_SETTINGS                            40301

#endif // _TAGSVIEW_RESOURCE_H_FILE_
