#ifndef _UEGUI_ROUTETYPESWITCHHOOK_H
#define _UEGUI_ROUTETYPESWITCHHOOK_H

#ifndef _UEGUI_BASE_H
#include "uegui.h"
#endif

#include "agghook.h"

//#include "uilabel.h"
#include "uibutton.h"
//#include "uiradiobutton.h"
//#include "uicheckbutton.h"

namespace UeGui
{
  typedef void (*RouteTypeCallBack) (CAggHook *sender, unsigned int planMethod);
  class UEGUI_CLASS CRouteTypeSwitchHook : public CAggHook
  {
  public:
    enum routetypeswitchhookCtrlType
    {
      routetypeswitchhook_Begin = 0,
      routetypeswitchhook_PopupListTop,
      routetypeswitchhook_PopupListBottom,
      routetypeswitchhook_RecommondBtn,
      routetypeswitchhook_RecommondBtnIcon,
      routetypeswitchhook_HighWayBtn,
      routetypeswitchhook_HighWayBtnIcon,
      routetypeswitchhook_EconomicWayBtn,
      routetypeswitchhook_EconomicWayBtnIcon,
      routetypeswitchhook_ShortestBtn,
      routetypeswitchhook_ShortestBtnIcon,
      routetypeswitchhook_Line1,
      routetypeswitchhook_Line2,
      routetypeswitchhook_Line3,
      routetypeswitchhook_GrayBack,
      routetypeswitchhook_End
    };

    CRouteTypeSwitchHook();

    virtual ~CRouteTypeSwitchHook();

    virtual short MouseDown(CGeoPoint<short> &scrPoint);

    virtual short MouseMove(CGeoPoint<short> &scrPoint);

    virtual short MouseUp(CGeoPoint<short> &scrPoint);

    virtual bool operator ()() { return true; }

    virtual void Load();
    static void SetRouteTypeCallBackFun(CAggHook *sender, RouteTypeCallBack callBack);

  protected:
    virtual tstring GetBinaryFileName();

    virtual void MakeNames();

    void MakeControls();

  private:
    void ChangeRouteType(unsigned int planMethod);

    
    void RouteReplan();

    void DoCallBack();

    void InitIconStatus();
  private:
    unsigned int m_planMethod;

    static RouteTypeCallBack m_callBackFun;
    static CAggHook *m_sender;
  private:
    CUiButton m_grayBackCtrl;
    CUiBitButton m_highWayBtnCtrl;
    CUiButton m_highWayIconCtrl;
    CUiBitButton m_shortestBtnCtrl;
    CUiButton m_shortestIconCtr;
    CUiButton m_popupListBottomCtrl;
    CUiButton m_popupListTopCtrl;
    CUiBitButton m_recommondBtnCtrl;
    CUiButton m_recommondIconCtrl;
    CUiBitButton m_economicWayBtnCtrl;
    CUiButton m_economicWayIconCtrl;
  };
}
#endif
