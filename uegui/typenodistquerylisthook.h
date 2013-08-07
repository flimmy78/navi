#ifndef _UEGUI_TYPENODISTQUERYLISTHOOK_H
#define _UEGUI_TYPENODISTQUERYLISTHOOK_H

#ifndef _UEGUI_BASE_H
#include "uegui.h"
#endif

#ifndef _UEQUERY_CODEINDEXCTRL_H
#include "uequery/codeindexctrl.h"
#endif

#include "uequery/uerecord.h"

#include "menubackgroundhook.h"

//#include "uilabel.h"
#include "uibutton.h"
//#include "uiradiobutton.h"
//#include "uicheckbutton.h"

namespace UeGui
{
  class UEGUI_CLASS CTypeNoDistQueryListHook : public CMenuBackgroundHook
  {
  
    enum RoundQueryType
    {
      MapCenter = 0,
      CurPos,
      EndPoint,
      Route
    };

  public:
    enum TypeNoDistQueryListHookCtrlType
    {
      TypeNoDistQueryListHook_Begin = MenuBackgroundHook_End,
      TypeNoDistQueryListHook_MapCenterBtn,
      TypeNoDistQueryListHook_CurPosBtn,
      TypeNoDistQueryListHook_EndPointBtn,
      TypeNoDistQueryListHook_RouteBtn,
      TypeNoDistQueryListHook_MapCenterFocusBtn,
      TypeNoDistQueryListHook_CurPosFocusBtn,
      TypeNoDistQueryListHook_EndPointFocusBtn,
      TypeNoDistQueryListHook_RouteFocusBtn,
      TypeNoDistQueryListHook_Bar,

      TypeNoDistQueryListHook_List1Btn,
      TypeNoDistQueryListHook_List1PoiNameBox,
      TypeNoDistQueryListHook_List1CursorBox,
      TypeNoDistQueryListHook_List1PoiDistBox,

      TypeNoDistQueryListHook_List2Btn,
      TypeNoDistQueryListHook_List2PoiNameBox,
      TypeNoDistQueryListHook_List2CursorBox,
      TypeNoDistQueryListHook_List2PoiDistBox,

      TypeNoDistQueryListHook_List3Btn,
      TypeNoDistQueryListHook_List3PoiNameBox,
      TypeNoDistQueryListHook_List3CursorBox,
      TypeNoDistQueryListHook_List3PoiDistBox,

      TypeNoDistQueryListHook_List4Btn,
      TypeNoDistQueryListHook_List4PoiNameBox,
      TypeNoDistQueryListHook_List4CursorBox,
      TypeNoDistQueryListHook_List4PoiDistBox,

      TypeNoDistQueryListHook_PageUpBtn,
      TypeNoDistQueryListHook_PageUpBtnIcon,
      TypeNoDistQueryListHook_PageDownBtn,
      TypeNoDistQueryListHook_PageDownBtnIcon,
      TypeNoDistQueryListHook_PageBack,
      TypeNoDistQueryListHook_CurPageInfo,
      TypeNoDistQueryListHook_AllPageInfo,
      TypeNoDistQueryListHook_PageInfoInterval,

      TypeNoDistQueryListHook_TypeSelectBtn,
      TypeNoDistQueryListHook_TypeSelectBtnIcon,
      TypeNoDistQueryListHook_TypeSelectBtnLabel,

      TypeNoDistQueryListHook_RadiusSelectBtn,
      TypeNoDistQueryListHook_RadiusSelectBtnIcon,
      TypeNoDistQueryListHook_RadiusSelectBtnLabel,

      TypeNoDistQueryListHook_End
    };

    CTypeNoDistQueryListHook();

    virtual ~CTypeNoDistQueryListHook();

    virtual short MouseDown(CGeoPoint<short> &scrPoint);

    virtual short MouseMove(CGeoPoint<short> &scrPoint);

    virtual short MouseUp(CGeoPoint<short> &scrPoint);

    virtual void Init();

    virtual void Load();

    virtual void UnLoad();

    void SetQueryTypeInfo(TCodeEntry *tcodeEntry);

  protected:

    virtual void MakeNames();

    void MakeControls();

    void SetFocusBtn();

    void SearchForResult();

    void ResetResultList();

  private:
    CUiButton m_mapCenterBtn;
    CUiButton m_curPosBtn;
    CUiButton m_endPointBtn;
    CUiButton m_routeBtn;

    CUiButton m_mapCenterFocusBtn;
    CUiButton m_curPosFocusBtn;
    CUiButton m_endPointFocusBtn;
    CUiButton m_routeFocusBtn;

    CUiBitButton m_typeSelectBtn;
    CUiBitButton m_radiusSelectBtn;

    CUiBitButton m_infoBtn[4];
    CUiLabel m_addrLabel[4];

    CUiBitButton m_pageUpBtn;
    CUiBitButton m_pageDownBtn;

    CUiLabel m_curPageInfo;
    CUiLabel m_totalPageInfo;

    CUeRecord* m_pRecord;

    PointList m_pointList;

    RoundQueryType m_queryType;

    TCodeEntry m_tCodeEntry;
    //刚在地图界面进入的时候记录地图中心点
    CGeoPoint<long> m_mapCenterPos;
  };
}
#endif
