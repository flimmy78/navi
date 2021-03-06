#ifndef _UEGUI_INPUTHANDHOOK_H
#define _UEGUI_INPUTHANDHOOK_H

#ifndef _UEGUI_BASE_H
#include "uegui.h"
#endif

#include "menubackgroundhook.h"

//#include "uilabel.h"
#include "uibutton.h"
//#include "uiradiobutton.h"
//#include "uicheckbutton.h"

#define INPUTCODENUM 10
#define MAXWORDNUM 20

namespace UeGui
{
  class UEGUI_CLASS CInputHandHook : public CMenuBackgroundHook
  {
  public:
    enum InputHandHookCtrlType
    {
      InputHandHook_Begin = MenuBackgroundHook_End,
      InputHandHook_CharBack,
      InputHandHook_InputCode1,
      InputHandHook_InputCode2,
      InputHandHook_InputCode3,
      InputHandHook_InputCode4,
      InputHandHook_InputCode5,
      InputHandHook_InputCode6,
      InputHandHook_InputCode7,
      InputHandHook_InputCode8,
      InputHandHook_InputCode9,
      InputHandHook_InputCode10,
      InputHandHook_WrittingArea,
      InputHandHook_WriteBackShade,
      InputHandHook_PageUpIcon,
      InputHandHook_PageDownIcon,
      InputHandHook_DistSwitchBtn,
      InputHandHook_DistSelectBtn,
      InputHandHook_PageUpBtn,
      InputHandHook_DistSelectBtnIcon,
      InputHandHook_PageDownBtn,
      InputHandHook_EditBox,
      InputHandHook_EditSelectBtn,
      InputHandHook_EditSelectBtnIcon,
      InputHandHook_KeyWordBox,
      InputHandHook_WordSeparation,
      InputHandHook_OtherSearchBtn,
      InputHandHook_InputSwitchBtn,
      InputHandHook_OtherSearchIcon,
      InputHandHook_InputSwitchBtnIcon,
      InputHandHook_DeleteBtn,
      InputHandHook_SearchBtn,
      InputHandHook_End
    };

    CInputHandHook();

    virtual ~CInputHandHook();

    virtual short MouseDown(CGeoPoint<short> &scrPoint);

    virtual short MouseMove(CGeoPoint<short> &scrPoint);

    virtual short MouseUp(CGeoPoint<short> &scrPoint);

    void ResetKeyWord(const char *pchKeyWord);

    char* GetKeyWord();

    void SetKeyWord(char* keyword);

    void SetQueryMode();

    //实时判断当前的手写输入
    void DoHandWriting(int curTime);

  protected:

    virtual void Load();

    virtual void UnLoad();

    virtual void Init();

    virtual void MakeNames();

    void MakeControls();

    bool EraseOneKeyWord();
    bool AddOneKeyWord(const char *pchLabelText);
    void ClearKeyWord(void);
    void SetCursorPosX(int curIndex);

    void ShowKeyWord();

    bool GetAssociateThing();

    void SetAssociateBtnLabels();

  private:
    //初始化手写输入
    static void InitHandWriting();
    //释放手写输入资源
    static void UninitHandWriting();

    static void DistSwitchCallBack(void *pDoCallBackObj, const SQLRecord *pResult);

    void DoDistSwitchCallBack(const SQLRecord *pResult);

    static void InputSelectCallBack(void *pDoCallBackObj, char *keyword);

    void DoInputSelectCallBack(char *keyword);

  private:
    CUiButton m_distSwitchBtn;
    CUiBitButton m_distSelectBtn;
    CUiBitButton m_editSelectBtn;
    CUiLabel m_keyWordBox;
    CUiBitButton m_otherSearchBtn;
    CUiButton m_searchBtn;
    CUiButton m_deleteBtn;
    CUiBitButton m_inputSwitchBtn;

    CUiButton m_inputCode[INPUTCODENUM];
    CUiBitButton m_pageDownBtn;
    CUiBitButton m_pageUpBtn;

    CUiBitButton m_returnBtn;

    int m_iCurCursorIndex;
    tstring m_tstrKeyWords;
    std::vector<int> m_vecWordPosX;
    //文字后面的白线
    GuiElement *m_pWordCursor;

    //当前显示在文字按钮上的文字索引
    int m_iCurWordIndex;
    //保存当前的要显示在按钮上的文字
    std::vector<string> m_vecWordsBuf;

    GuiElement *m_pWrittingArea;

    //因为所有的手写屏幕一样大，且不会同时出现
    // Handwriting library
    //手写输入字库
    static unsigned char *m_hwAddress;
    //用来保存手写输入时的点坐标
    static short *m_writingPts;
    //保存坐标点的缓存大小
    static short m_writingNum;
    //当前可以保存坐标的下标
    static short m_writingCursor;
    //保存搜索poi和路名用的关键字
    char m_poiKeyWord[128];
    //保存搜索区域用的关键字
    char m_distKeyWord[128];
    //
    bool m_isNewChar;
    int m_writingTime;
    short m_prevX;
    short m_prevY;
    bool m_isWriting;

    HPEN m_pen;
    HPEN m_oldPen;
    HDC m_renderingDC;
    //判断当前是准备纠正想要输入的字还是输入下一个联想字
    bool m_isIdentify;

    //关键字过长显示不全时, 标识从第几个字开始显示
    int m_wordPosOffset;
    //用于保存截取后用于显示的字符串
    tstring m_cutKeyWords;
    //记录关键字输入框的原始位置
    int m_orinBoxPos;
    //光标可以移动到的最后位置
    int m_limitPoxX;
    //记录当前是否显示完整的关键字
    bool m_isShowFullKeyWords;
  };
}
#endif
