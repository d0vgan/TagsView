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
#define IDC_CH_OPT_CTAGSSTDOUT                  2021
#define IDM_PREVPOS                             40000
#define IDM_NEXTPOS                             40001
#define IDM_SORTLINE                            40002
#define IDM_SORTNAME                            40003
#define IDM_VIEWLIST                            40004
#define IDM_VIEWTREE                            40005
#define IDM_PARSE                               40006
#define IDM_CLOSE                               40007

#endif // _TAGSVIEW_RESOURCE_H_FILE_
