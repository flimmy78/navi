/*
*
*/
#include "ueroute\routebasic.h"
#include "uebase\timebasic.h"
#include "uemap\aggview.h"
#include "gui.h"
#include "maphook.h"
#include "viewwrapper.h"
#include "routewrapper.h"
#include "querywrapper.h"
#include "userdatawrapper.h"
#include "settingwrapper.h"
#include "messagedialoghook.h"
#include "inputswitchhook.h"
#include "detailmessagehook.h"
#include "usuallyfile.h"
#include "itemselecthook.h"
#include "startuphook.h"

using namespace UeGui;
//
#define RENDERCAMERABTN 1
#define IS_SCROLL_ON   0

/////////////////////////////////////////////////////////////////////////////////////////////

UeGui::CMapHook::CMapHook() : CAggHook(), m_viewWrapper(CViewWrapper::Get()), m_routeWrapper(CRouteWrapper::Get()),
m_settingWrapper(CSettingWrapper::Get()), m_userDataWrapper(CUserDataWrapper::Get()), m_queryWrapper(CQueryWrapper::Get()),
m_selectPointObj(NULL), m_selectPointEvent(NULL), m_curGuiType(GUI_None), m_preGuiType(GUI_None), m_queryPointIndex(-1), 
m_planType(Plan_Single),m_guiTimerInterval(0), m_curPlanmethodType(UeRoute::MT_Max), m_elecEyeStatus(false), m_bIsCompassShown(false),
m_elecProgressBarWidth(0), m_elecMaxPromptDist(0), m_screenMode(SM_None), m_needRestoreRoute(false), m_lanHight(50), m_lanWidth(0), 
m_firstDrawMap(true), m_sysTime(0), m_downTimeCount(0), m_bIsShowShortcutPanel(false)
{
  m_restorePoiList.clear();
  m_queryPointList.clear();
  //地图界面渲染完成后不需要释放图片资源，提高效率
  m_needReleasePic = false;
  
  CItemSelectHook::ItemInfo itemInfo;
  //初始化屏幕模式列表
  ::strcpy(itemInfo.m_itemName, "鹰眼图");
  itemInfo.m_enable = true;
  itemInfo.m_showIcon = false;
  m_srcModalItemList.push_back(itemInfo);
  ::strcpy(itemInfo.m_itemName, "后续路口");
  itemInfo.m_enable = true;
  itemInfo.m_showIcon = false;
  m_srcModalItemList.push_back(itemInfo);
  ::strcpy(itemInfo.m_itemName, "高速看板");
  itemInfo.m_enable = true;
  itemInfo.m_showIcon = false;
  m_srcModalItemList.push_back(itemInfo);
  //::strcpy(itemInfo.m_itemName, "一般双屏");
  //itemInfo.m_enable = false;
  //itemInfo.m_showIcon = false;
  //m_srcModalItemList.push_back(itemInfo);

  //初始化快捷比例尺名称列表  
  ::strcpy(itemInfo.m_itemName, "街道");
  itemInfo.m_enable = true;
  itemInfo.m_showIcon = false;
  m_scaleItemList.push_back(itemInfo);
  ::strcpy(itemInfo.m_itemName, "城市");
  itemInfo.m_enable = true;
  itemInfo.m_showIcon = false;
  m_scaleItemList.push_back(itemInfo);
  ::strcpy(itemInfo.m_itemName, "省");
  itemInfo.m_enable = true;
  itemInfo.m_showIcon = false;
  m_scaleItemList.push_back(itemInfo);
  ::strcpy(itemInfo.m_itemName, "全国");
  itemInfo.m_enable = true;
  itemInfo.m_showIcon = false;
  m_scaleItemList.push_back(itemInfo);
}

UeGui::CMapHook::~CMapHook()
{
  m_elements.clear();
  m_ctrlNames.clear();
  m_imageNames.clear();
}

bool UeGui::CMapHook::operator()()
{
  return false;
}

void UeGui::CMapHook::DoDraw( const CGeoRect<short> &scrExtent )
{
  //调用基类渲染界面
  CAggHook::DoDraw(scrExtent);

  //判断是否是第一次渲染，并且只进入一次
  if (m_firstDrawMap)
  {
    //只有渲染过一次地图之后，才能读取到地图上的点、线、面数据
    m_firstDrawMap = false;
    if (m_bIsShowShortcutPanel)
    {
      CStartUpHook* startUpHook = (CStartUpHook*)m_viewWrapper.GetHook(CViewHook::DHT_StartUpHook);
      if (startUpHook)
      {
        startUpHook->SetRestoreRouteState(m_needRestoreRoute);
        TurnTo(DHT_StartUpHook);
        Refresh();
      }      
    }
    else if (m_needRestoreRoute)
    {
      //打开提示回复路线对话框
      CMessageDialogEvent dialogEvent(this, DHT_MapHook, &CMapHook::OnRestoreRote);
      CMessageDialogHook::ShowMessageDialog(MB_WARNING, "是否要回复上次路线吗?", dialogEvent);   
    }
  }
}

void UeGui::CMapHook::Init()
{
  //读取是否显示快捷面板
  m_bIsShowShortcutPanel = m_settingWrapper.GetIsShowShortcutPanel();
  //读取是否需要回复上次未导航完成路线
  m_needRestoreRoute = m_userDataWrapper.GetLastRoute(m_restoreRouteType, m_restorePoiList);
  
  RefreshSetPointStatus();
  RefreshZoomingInfo();
  //读取开机位置
  UsuallyRecord usuallData;
  int rt = m_userDataWrapper.GetUsuallyRecord(RT_STARTPOS ,&usuallData);
  if ((0 == rt) && (usuallData.IsValid()))
  {
    CGeoPoint<long> mapPoint;
    CGeoPoint<short> scrPoint;
    mapPoint.m_x = usuallData.m_x;
    mapPoint.m_y = usuallData.m_y;
    m_viewWrapper.SetPickPos(mapPoint, scrPoint, true);
    UeMap::GpsCar gpsCar;
    gpsCar.m_curPos.m_x = mapPoint.m_x;
    gpsCar.m_curPos.m_y = mapPoint.m_y;
    m_viewWrapper.SetGpsPosInfo(gpsCar);
    m_viewWrapper.SetGpsCar(gpsCar); 
    RefreshLocationInfo((const char*)usuallData.m_name);
  }
  else
  {
    //如果没有设置开机位置则读取最后保存的GPS位置
    CViewState* curViewState = m_viewWrapper.GetMainViewState();
    if (curViewState)
    {
      //curViewState->RefreshLayerData();
      const ScreenLayout& srcLayout = curViewState->GetScrLayout();
      CGeoPoint<short> scrPoint = srcLayout.m_base;
      //屏幕选点，读取当前选点的名称
      CMemVector objects(sizeof(CViewCanvas::RenderedPrimitive), 10);
      m_viewWrapper.Pick(scrPoint, objects, false);
      RefreshLocationInfo();
    }   
  }
  //如果当前地图模式为引导状态则切换成浏览状态
  m_viewWrapper.SetViewOpeMode(VM_Guidance);
}

void UeGui::CMapHook::Update( short type )
{
  switch(type)
  {
  case CViewHook::UHT_FillGuidanceInfo:
    {
      FillGuidanceInfo();
      UpdateElecEyeInfo();
      break;
    }
  case CViewHook::UHT_UpdateLocationMapHook:
    {
      RefreshLocationInfo();
      break;
    }
  case CViewHook::UHT_UpdateMapHook:
    {
      //读取当前规划状态
      short planState = m_routeWrapper.GetPlanState();
      if ((UeRoute::PS_None == planState) || (UeRoute::PS_Ready == planState))
      {
        //如果是无路线状态则证明当前导航结束
        DoStopGuide();
        m_viewWrapper.Refresh();
      }
      else
      {
        //否则更新菜单
        UpdateMenu();
      }
      break;
    }
  }
}

void UeGui::CMapHook::MakeGUI()
{
  //调用父类接口
  CAggHook::MakeGUI();
  //非导航和真实导航时菜单
  m_mapMainMenu.MakeGUI();
  m_mapMainMenu.ExpandMenu(false);
  m_mapMainMenu.Show(false);
  m_mapMainMenu.SetParentHook(this);
  AddChildHook(CViewHook::DHT_MapMainMenuHook, &m_mapMainMenu);
  //模拟导航时菜单
  m_mapSimulationMenu.MakeGUI();
  m_mapSimulationMenu.Show(false);
  m_mapSimulationMenu.SetParentHook(this);
  AddChildHook(CViewHook::DHT_MapSimulationMenuHook, &m_mapSimulationMenu);
  //路线计算完成后菜单
  m_mapRouteCalcMenu.MakeGUI();
  m_mapRouteCalcMenu.Show(false);
  m_mapRouteCalcMenu.SetParentHook(this);
  AddChildHook(CViewHook::DHT_MapRouteCalcMenuHook, &m_mapRouteCalcMenu);
  //概览路线菜单
  m_mapOverViewMenu.MakeGUI();
  m_mapOverViewMenu.Show(false);
  m_mapOverViewMenu.SetParentHook(this);
  AddChildHook(CViewHook::DHT_MapOverViewMenuHook, &m_mapOverViewMenu);
  //引导视图菜单：下一道路、方向看板、红绿灯等
  m_mapGuideInfoView.MakeGUI();
  m_mapGuideInfoView.Show(false);
  m_mapGuideInfoView.SetParentHook(this);
  AddChildHook(CViewHook::DHT_MapGuideViewMenuHook, &m_mapGuideInfoView);
  //查询时菜单
  m_mapQueryMenu.MakeGUI();
  m_mapQueryMenu.Show(false);
  m_mapQueryMenu.SetParentHook(this);
  AddChildHook(CViewHook::DHT_MapQueryViewMenuHook, &m_mapQueryMenu);

  //初始化地图模式为引导状态
  m_viewWrapper.SetViewOpeMode(VM_Guidance);
  //初始化菜单
  SwitchingGUI(GUI_MapBrowse);
}

tstring UeGui::CMapHook::GetBinaryFileName()
{
  return _T("maphook.bin");
}

void UeGui::CMapHook::MakeNames()
{
  m_ctrlNames.erase(m_ctrlNames.begin(), m_ctrlNames.end());
  m_ctrlNames.insert(GuiName::value_type(MapHook_MiniMizeBack,	"MiniMizeBack"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_MiniMizeIcon,	"MiniMizeIcon"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_AddElecEyeBack,	"AddElecEyeBack"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_AddElecEyeIcon,	"AddElecEyeIcon"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_MapAzimuthBack,	"MapAzimuthBack"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_NorthIcon,	"NorthIcon"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_HeadingIcon,	"HeadingIcon"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_PerspectiveIcon,	"PerspectiveIcon"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_CompassBack,	"CompassBack"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_CompassIcon,	"CompassIcon"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_ZoomInBack,	"ZoomInBack"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_ZoomInIcon,	"ZoomInIcon"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_ZoomOutBack,	"ZoomOutBack"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_ZoomOutIcon,	"ZoomOutIcon"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_ScaleBack,	"ScaleBack"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_ScaleIcon,	"ScaleIcon"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_ScaleLabel,	"ScaleLabel"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_SoundBack,	"SoundBack"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_SoundIcon,	"SoundIcon"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_GPSBack,	"GPSBack"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_GPSIcon,	"GPSIcon"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_GPSIcon0,	"GPSIcon0"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_GPSIcon1,	"GPSIcon1"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_GPSIcon2,	"GPSIcon2"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_GPSIcon4,	"GPSIcon4"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_GPSIcon3,	"GPSIcon3"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_GPSIcon5,	"GPSIcon5"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_GPSIcon6,	"GPSIcon6"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_GPSIcon7,	"GPSIcon7"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_GPSIcon8,	"GPSIcon8"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_GPSIcon9,	"GPSIcon9"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_GPSIcon10,	"GPSIcon10"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_GPSIcon11,	"GPSIcon11"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_GPSIcon12,	"GPSIcon12"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_ScreenMoadlBack,	"ScreenMoadlBack"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_SingleScreenIcon,	"SingleScreenIcon"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_DoubleScreenIcon,	"DoubleScreenIcon"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_SetStartBack,	"SetStartBack"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_SetStartIcon,	"SetStartIcon"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_SetStartLabel,	"SetStartLabel"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_SetWayBack,	"SetWayBack"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_SetWayIcon,	"SetWayIcon"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_SetWayLabel,	"SetWayLabel"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_SetEndBack,	"SetEndBack"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_SetEndIcon,	"SetEndIcon"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_SetEndLabel,	"SetEndLabel"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_FixedPostionBack,	"FixedPostionBack"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_FixedPostionIcon,	"FixedPostionIcon"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_FixedPostionLabel,	"FixedPostionLabel"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_TimerLabel,	"TimerLabel"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_DetailBack1,	"DetailBack1"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_DetailIcon1,	"DetailIcon1"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_DetailLabe1,	"DetailLabe1"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_DetailBack2,	"DetailBack2"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_DetailIcon2,	"DetailIcon2"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_DetailLabe2,	"DetailLabe2"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_GuideInfoLeftBack,	"GuideInfoLeftBack"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_GuideInfoLeftIcon,	"GuideInfoLeftIcon"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_GuideInfoLeftLabe,	"GuideInfoLeftLabe"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_Delimiter1,	"Delimiter1"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_GuideInfoCenterBack,	"GuideInfoCenterBack"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_GuideInfoCenterIcon,	"GuideInfoCenterIcon"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_GuideInfoCenterLabe,	"GuideInfoCenterLabe"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_Delimiter2,	"Delimiter2"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_GuideInfoRightBack,	"GuideInfoRightBack"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_GuideInfoRightIcon,	"GuideInfoRightIcon"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_GuideInfoRightLabe,	"GuideInfoRightLabe"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_ElecEyeBack,	"ElecEyeBack"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_ElecEyeIcon,	"ElecEyeIcon"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_ElecEyeProgressBar,	"ElecEyeProgressBar"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_ElecEyeIconType_1,	"ElecEyeIconType_1"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_ElecEyeIconType_2,	"ElecEyeIconType_2"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_ElecEyeIconType_3,	"ElecEyeIconType_3"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_ElecEyeIconType_4,	"ElecEyeIconType_4"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_ElecEyeIconType_5,	"ElecEyeIconType_5"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_ElecEyeIconType_6,	"ElecEyeIconType_6"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_ElecEyeIconType_7,	"ElecEyeIconType_7"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_ElecEyeIconType_8,	"ElecEyeIconType_8"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_ElecEyeIconType_9,	"ElecEyeIconType_9"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_ElecEyeIconType_10,	"ElecEyeIconType_10"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_ElecEyeIconType_11,	"ElecEyeIconType_11"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_ElecEyeIconType_12,	"ElecEyeIconType_12"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_ElecEyeIconType_13,	"ElecEyeIconType_13"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_OtherIcon1,	"OtherIcon1"));
  m_ctrlNames.insert(GuiName::value_type(MapHook_OtherIcon2,	"OtherIcon2"));
}

void UeGui::CMapHook::MakeControls()
{
  m_miniMizeBtn.SetCenterElement(GetGuiElement(MapHook_MiniMizeBack));
  m_miniMizeBtn.SetIconElement(GetGuiElement(MapHook_MiniMizeIcon));
  m_addElecEyeBtn.SetCenterElement(GetGuiElement(MapHook_AddElecEyeBack));
  m_addElecEyeBtn.SetIconElement(GetGuiElement(MapHook_AddElecEyeIcon));
  m_mapAzimuthBtn.SetCenterElement(GetGuiElement(MapHook_MapAzimuthBack));
  m_mapAzimuthBtn.SetIconElement(GetGuiElement(MapHook_NorthIcon));
  m_zoomInBtn.SetCenterElement(GetGuiElement(MapHook_ZoomInBack));
  m_zoomInBtn.SetIconElement(GetGuiElement(MapHook_ZoomInIcon));
  m_zoomOutBtn.SetCenterElement(GetGuiElement(MapHook_ZoomOutBack));
  m_zoomOutBtn.SetIconElement(GetGuiElement(MapHook_ZoomOutIcon));
  m_scaleBtn.SetCenterElement(GetGuiElement(MapHook_ScaleBack));
  m_scaleBtn.SetIconElement(GetGuiElement(MapHook_ScaleIcon));
  m_scaleBtn.SetLabelElement(GetGuiElement(MapHook_ScaleLabel));
  m_soundBtn.SetCenterElement(GetGuiElement(MapHook_SoundBack));
  m_soundBtn.SetIconElement(GetGuiElement(MapHook_SoundIcon));
  m_GPSBtn.SetCenterElement(GetGuiElement(MapHook_GPSBack));
  m_GPSBtn.SetIconElement(GetGuiElement(MapHook_GPSIcon));
  m_screenMoadlBtn.SetCenterElement(GetGuiElement(MapHook_ScreenMoadlBack));
  m_screenMoadlBtn.SetIconElement(GetGuiElement(MapHook_SingleScreenIcon));
  m_setStartBtn.SetCenterElement(GetGuiElement(MapHook_SetStartBack));
  m_setStartBtn.SetIconElement(GetGuiElement(MapHook_SetStartIcon));
  m_setStartBtn.SetLabelElement(GetGuiElement(MapHook_SetStartLabel));
  m_setEndBtn.SetCenterElement(GetGuiElement(MapHook_SetEndBack));
  m_setEndBtn.SetIconElement(GetGuiElement(MapHook_SetEndIcon));
  m_setEndBtn.SetLabelElement(GetGuiElement(MapHook_SetEndLabel));
  m_setWayBtn.SetCenterElement(GetGuiElement(MapHook_SetWayBack));
  m_setWayBtn.SetIconElement(GetGuiElement(MapHook_SetWayIcon));
  m_setWayBtn.SetLabelElement(GetGuiElement(MapHook_SetWayLabel));
  m_fixedPostionBtn.SetCenterElement(GetGuiElement(MapHook_FixedPostionBack));
  m_fixedPostionBtn.SetIconElement(GetGuiElement(MapHook_FixedPostionIcon));
  m_fixedPostionBtn.SetLabelElement(GetGuiElement(MapHook_FixedPostionLabel));
  //系统时间
  m_timerBtn.SetCenterElement(GetGuiElement(MapHook_TimerLabel));
  //指南针
  m_compassIcon.SetCenterElement(GetGuiElement(MapHook_CompassBack));  
  m_compassIcon.SetIconElement(GetGuiElement(MapHook_CompassIcon));
  //详情
  m_detailBtn1.SetCenterElement(GetGuiElement(MapHook_DetailBack1));
  m_detailBtn1.SetIconElement(GetGuiElement(MapHook_DetailIcon1));
  m_detailBtn1.SetLabelElement(GetGuiElement(MapHook_DetailLabe1));
  m_detailBtn2.SetCenterElement(GetGuiElement(MapHook_DetailBack2));
  m_detailBtn2.SetIconElement(GetGuiElement(MapHook_DetailIcon2));
  m_detailBtn2.SetLabelElement(GetGuiElement(MapHook_DetailLabe2));
  //详情
  m_guideInfoLeftBtn.SetCenterElement(GetGuiElement(MapHook_GuideInfoLeftBack));
  m_guideInfoLeftBtn.SetIconElement(GetGuiElement(MapHook_GuideInfoLeftIcon));
  m_guideInfoLeftBtn.SetLabelElement(GetGuiElement(MapHook_GuideInfoLeftLabe));
  m_guideInfoCenterBtn.SetCenterElement(GetGuiElement(MapHook_GuideInfoCenterBack));
  m_guideInfoCenterBtn.SetIconElement(GetGuiElement(MapHook_GuideInfoCenterIcon));
  m_guideInfoCenterBtn.SetLabelElement(GetGuiElement(MapHook_GuideInfoCenterLabe));
  m_guideInfoRightBtn.SetCenterElement(GetGuiElement(MapHook_GuideInfoRightBack));
  m_guideInfoRightBtn.SetIconElement(GetGuiElement(MapHook_GuideInfoRightIcon));
  m_guideInfoRightBtn.SetLabelElement(GetGuiElement(MapHook_GuideInfoRightLabe));
  //分割符
  m_delimiter1.SetCenterElement(GetGuiElement(MapHook_Delimiter1));
  m_delimiter2.SetCenterElement(GetGuiElement(MapHook_Delimiter2));
  //电子眼提示图标
  m_elecEye.SetCenterElement(GetGuiElement(MapHook_ElecEyeBack));
  m_elecEye.SetIconElement(GetGuiElement(MapHook_ElecEyeIcon));
  m_elecEye.SetLabelElement(GetGuiElement(MapHook_ElecEyeProgressBar));

  CViewHook::GuiElement* guiElement = NULL;
  //读取电子眼进度条的宽度
  guiElement = m_elecEye.GetLabelElement();
  if (guiElement)
  {
    m_elecProgressBarWidth = guiElement->m_width;
  }

  //设置指南针图标
  guiElement = m_compassIcon.GetCenterElement();
  if (guiElement)
  {
    CGeoPoint<short> scrPoint;
    scrPoint.m_x = guiElement->m_startX + guiElement->m_width/2;
    scrPoint.m_y = guiElement->m_startY + guiElement->m_height/2;
    //设置指南针显示图标
    m_viewWrapper.AddViewIcon(VI_COMPASSICON, guiElement->m_bkNormal);
  }
  guiElement = m_compassIcon.GetIconElement();
  if (guiElement)
  {    
    m_viewWrapper.AddViewIcon(VI_COMPASSINDICATORICON, guiElement->m_bkNormal);
    //设置指南针的位置  
    CGeoPoint<short> scrPoint;
    scrPoint.m_x = guiElement->m_startX + guiElement->m_width / 2;
    scrPoint.m_y = guiElement->m_startY + guiElement->m_height / 2;
    m_view->SetCompassIconPos(scrPoint);
  }
  
  guiElement = GetGuiElement(MapHook_OtherIcon1);
  if (guiElement)
  {
    //3D白天天空图片
    m_viewWrapper.AddViewIcon(VI_SKY_DAY_ICON, guiElement->m_bkNormal);
    //3D夜晚天空图片
    m_viewWrapper.AddViewIcon(VI_SKY_NIGHT_ICON, guiElement->m_bkFocus);
  }

  guiElement = GetGuiElement(MapHook_OtherIcon2);
  if (guiElement)
  {
    //设置路口气泡图标
    m_viewWrapper.AddViewIcon(VI_BUBBLEICON, guiElement->m_bkNormal);
    //设置电子眼图标
    m_viewWrapper.AddViewIcon(VI_ELECTRONIC_ICON, guiElement->m_bkFocus);
  }

  guiElement = m_soundBtn.GetCenterElement();
  if (guiElement)
  {
      m_lanHight = guiElement->m_height - 2;
      m_lanWidth = guiElement->m_height - 2;
  }
  //设置车道位置
  ResetLanPos();
}

void UeGui::CMapHook::Timer()
{
  //长按滚动
  short planState = m_routeWrapper.GetPlanState();
  if (m_downElementType == CT_Unknown && planState != UeRoute::PS_DemoGuidance && planState != UeRoute::PS_RealGuidance )
  {
    //长按超过1秒即滚动
    if (IsMouseDown() && m_downTimeCount > 0) 
    {
      CViewState *curState = m_viewWrapper.GetMainViewState(); 
      //启动线程进行滚动
      curState->ScrollMap();
    }
    else
    {
      m_downTimeCount = 0;
    }
    if (IsMouseDown() && m_downTimeCount < 2)
    {
      m_downTimeCount++;
    }
  }

  //界面定时切换
  if (m_guiTimerInterval > 0)
  {
    --m_guiTimerInterval;
    if (0 == m_guiTimerInterval)
    {
      if (UpdateMenu(MUT_Close))
      {
        Refresh();
      }      
    }
  }

  //路径规划自动开始导航倒计时
  if (m_mapRouteCalcMenu.IsShown())
  {
    m_mapRouteCalcMenu.Timer();
  }

  //刷新系统时间
  RefreshSysTime();
}

void UeGui::CMapHook::Load()
{

}

void UeGui::CMapHook::UnLoad()
{

}

void UeGui::CMapHook::SwitchingGUI( GUIType guiType )
{
  //保存当前切换的界面类型
  m_preGuiType = m_curGuiType;
  m_curGuiType = guiType;
  //根据界面类型显示菜单
  switch (guiType)
  {
  case GUI_MapBrowse:
    {
      m_mapMainMenu.Show();
      m_mapSimulationMenu.Show(false);
      m_mapRouteCalcMenu.Show(false);
      m_mapOverViewMenu.Show(false);
      m_mapGuideInfoView.Show(false);
      m_mapQueryMenu.Show(false);
      break;
    }
  case GUI_RouteCalculate:
    {
      m_mapRouteCalcMenu.Show();      
      m_mapMainMenu.Show(false);
      m_mapSimulationMenu.Show(false);
      m_mapOverViewMenu.Show(false);
      m_mapGuideInfoView.Show(false);
      m_mapQueryMenu.Show(false);
      CloseGuiTimer();
      break;
    }
  case GUI_DemoGuidance:
    {
      m_mapSimulationMenu.Show();
      m_mapGuideInfoView.Show();
      m_mapMainMenu.Show(false);
      m_mapRouteCalcMenu.Show(false);
      m_mapOverViewMenu.Show(false);
      m_mapQueryMenu.Show(false);
      break;
    }
  case GUI_RealGuidance:
    {
      m_mapMainMenu.Show();
      m_mapGuideInfoView.Show();
      m_mapRouteCalcMenu.Show(false);
      m_mapSimulationMenu.Show(false);
      m_mapOverViewMenu.Show(false);
      m_mapQueryMenu.Show(false);
      break;
    }
  case GUI_OverviewRoute:
    {
      m_mapOverViewMenu.Show();
      m_mapMainMenu.Show(false);
      m_mapGuideInfoView.Show(false);
      m_mapRouteCalcMenu.Show(false);
      m_mapSimulationMenu.Show(false);      
      m_mapQueryMenu.Show(false);
      CloseGuiTimer();
      break;
    }
  case GUI_QueryPoint:
    {
      m_mapQueryMenu.SetMenuType(CMapQueryMenuHook::QMenu_QueryPoint);
      m_mapQueryMenu.Show();
      m_mapMainMenu.Show(false);
      m_mapRouteCalcMenu.Show(false);
      m_mapSimulationMenu.Show(false);
      m_mapOverViewMenu.Show(false);
      m_mapGuideInfoView.Show(false);
      break;
    }
  case GUI_SelectPoint:
    {
      m_mapQueryMenu.SetMenuType(CMapQueryMenuHook::QMenu_SelectPoint);
      m_mapQueryMenu.Show();
      m_mapMainMenu.Show(false);
      m_mapRouteCalcMenu.Show(false);
      m_mapSimulationMenu.Show(false);
      m_mapOverViewMenu.Show(false);
      m_mapGuideInfoView.Show(false);
      CloseGuiTimer();
      break;
    }
  case GUI_SelectArea:
    {
      m_mapQueryMenu.SetMenuType(CMapQueryMenuHook::QMenu_SelectArea);
      m_mapQueryMenu.Show();
      m_mapMainMenu.Show(false);
      m_mapRouteCalcMenu.Show(false);
      m_mapSimulationMenu.Show(false);
      m_mapOverViewMenu.Show(false);
      m_mapGuideInfoView.Show(false);
      CloseGuiTimer();
      break;
    }
  }
}

void UeGui::CMapHook::ReturnToPrevGUI()
{
  SwitchingGUI(m_preGuiType);
}

bool UeGui::CMapHook::UpdateMenu( MenuUpdateType updateType /*= MUT_Normal*/ )
{
  bool rt = false;
  switch (updateType)
  {
  case MUT_Normal:
    {
      if (m_mapMainMenu.IsShown())
      {
        m_mapMainMenu.Update(updateType);
        rt = true;
      }
      if (m_mapSimulationMenu.IsShown())
      {
        m_mapSimulationMenu.Update(updateType);
        rt = true;
      }
      if (m_mapGuideInfoView.IsShown())
      {
        m_mapGuideInfoView.Update(updateType);
        rt = true;
      }      
      break;
    }
  case MUT_Expand:
    {
      if (m_mapMainMenu.IsShown())
      {
        m_mapMainMenu.ExpandMenu();
        rt = true;
      }
      if (m_mapSimulationMenu.IsShown())
      {
        m_mapSimulationMenu.ExpandMenu();
        rt = true;
      }
      if (m_mapQueryMenu.IsShown())
      {
        m_mapQueryMenu.ExpandMenu();
        rt = true;
      }
      break;
    }
  case MUT_Close:
    {
      if (m_mapMainMenu.IsShown())
      {
        m_mapMainMenu.ExpandMenu(false);
        rt = true;
      }
      if (m_mapSimulationMenu.IsShown())
      {
        m_mapSimulationMenu.ExpandMenu(false);
        rt = true;
      }
      if (m_mapQueryMenu.IsShown())
      {
        m_mapQueryMenu.ExpandMenu(false);
        rt = true;
      }
      break;
    }
  }
  return rt;
}

void UeGui::CMapHook::MapTouch( CGeoPoint<short> &scrPoint )
{
  //如果是模拟导航状态则不允许操作地图
  short planState = m_routeWrapper.GetPlanState();
  if (UeRoute::PS_DemoGuidance == planState)
  {
    //展开菜单
    if (UpdateMenu(MUT_Expand))
    {
      m_viewWrapper.RefreshUI();
    }
    return;
  }
  //如果当前地图模式为引导状态则切换成浏览状态
  CViewState* curViewState = m_viewWrapper.GetMainViewState();
  if (curViewState)
  {
    UeMap::ViewOpeMode viewMode = m_viewWrapper.GetViewOpeMode((UeMap::ViewType)curViewState->GetType());
    if (UeMap::VM_Guidance == viewMode)                                    
    {
      //切换地图为可操作状态
      m_viewWrapper.SetViewOpeMode(VM_Browse);
      //如果是真实导航界面，则切换到浏览状态
      if (GUI_RealGuidance == m_curGuiType)
      {    
        SwitchingGUI(GUI_MapBrowse);
      }
      //展开菜单
      if (UpdateMenu(MUT_Expand))
      { 
        Refresh();
      }
    }
  }
  //地图选点
  unsigned int viewType = m_view->GetSelectedViewType(scrPoint);
  if ((VT_Heading == viewType) || (VT_North == viewType) || (VT_Perspective == viewType))
  {
    CMemVector objects(sizeof(CViewCanvas::RenderedPrimitive), 10);
    m_viewWrapper.Pick(scrPoint, objects, true);
    RefreshLocationInfo();
  }
}

short UeGui::CMapHook::MouseDown(CGeoPoint<short> &scrPoint)
{
  //是否需要刷新
  bool needRefresh = false;
  short ctrlType = CAggHook::MouseDown(scrPoint);
  switch(m_downElementType)
  {
  case CT_Unknown:
    {
      return CT_Unknown;
    }
    break;
  case MapHook_MiniMizeBack:
  case MapHook_MiniMizeIcon:
    {
      m_miniMizeBtn.MouseDown();
      needRefresh = true;
    }
    break;
  case MapHook_AddElecEyeBack:
  case MapHook_AddElecEyeIcon:
    {
      m_addElecEyeBtn.MouseDown();
      needRefresh = true;
    }
    break;
  case MapHook_MapAzimuthBack:
  case MapHook_NorthIcon:
  case MapHook_PerspectiveIcon:
  case MapHook_HeadingIcon:
    {
      m_mapAzimuthBtn.MouseDown();
      needRefresh = true;
    }
    break;
  case MapHook_ZoomInBack:
  case MapHook_ZoomInIcon:
    {
      m_zoomInBtn.MouseDown();
      needRefresh = true;
    }
    break;
  case MapHook_ZoomOutBack:
  case MapHook_ZoomOutIcon:
    {
      m_zoomOutBtn.MouseDown();
      needRefresh = true;
    }
    break;
  case MapHook_ScaleBack:
  case MapHook_ScaleLabel:
    {
      m_scaleBtn.MouseDown();
      needRefresh = true;
    }
    break;
  case MapHook_SoundBack:
  case MapHook_SoundIcon:
    {
      m_soundBtn.MouseDown();
      needRefresh = true;
    }
    break;
  case MapHook_GPSBack:
  case MapHook_GPSIcon:
    {
      m_GPSBtn.MouseDown();
      needRefresh = true;
    }
    break;
  case MapHook_ScreenMoadlBack:
  case MapHook_SingleScreenIcon:
  case MapHook_DoubleScreenIcon:
    {
      m_screenMoadlBtn.MouseDown();
      needRefresh = true;
    }
    break;
  case MapHook_SetStartBack:
  case MapHook_SetStartIcon:
  case MapHook_SetStartLabel:
    {
      m_setStartBtn.MouseDown();
      needRefresh = true;
    }
    break;
  case MapHook_SetEndBack:
  case MapHook_SetEndIcon:
  case MapHook_SetEndLabel:
    {
      m_setEndBtn.MouseDown();
      needRefresh = true;
    }
    break;
  case MapHook_SetWayBack:
  case MapHook_SetWayIcon:
  case MapHook_SetWayLabel:
    {
      m_setWayBtn.MouseDown();
      needRefresh = true;
    }
    break;
  case MapHook_FixedPostionBack:
  case MapHook_FixedPostionIcon:
  case MapHook_FixedPostionLabel:
    {
      m_fixedPostionBtn.MouseDown();
      needRefresh = true;
    }
    break;
  case MapHook_DetailBack1:
  case MapHook_DetailIcon1:
  case MapHook_DetailLabe1:
    {
      m_detailBtn1.MouseDown();
      needRefresh = true;
    }
    break;
  case MapHook_DetailBack2:
  case MapHook_DetailIcon2:
  case MapHook_DetailLabe2:
    {
      m_detailBtn2.MouseDown();
      needRefresh = true;
    }
    break;
  case MapHook_GuideInfoCenterBack:
  case MapHook_GuideInfoCenterIcon:
  case MapHook_GuideInfoCenterLabe:
    {
      m_guideInfoCenterBtn.MouseDown();
      needRefresh = true;
    }
    break;
  default:
    assert(false);
    break;
  }

  //如果鼠标点击的不是地图则停止计时
  if (CT_Unknown != m_downElementType)
  {
    CloseGuiTimer();  
  }
  if (needRefresh)
  {
    Refresh();
  }
  return ctrlType;
}

short UeGui::CMapHook::MouseMove(CGeoPoint<short> &scrPoint)
{
  return -1;
}

short UeGui::CMapHook::MouseUp(CGeoPoint<short> &scrPoint)
{
  //是否需要刷新
  bool needRefresh = false;
  short ctrlType = CAggHook::MouseUp(scrPoint);
  switch(m_downElementType)
  {
  case CT_Unknown:
    {
      //处理点击地图事件
      MapTouch(scrPoint);
    }
    break;
  case MapHook_MiniMizeBack:
  case MapHook_MiniMizeIcon:
    {
      //隐藏
      m_miniMizeBtn.MouseUp();
      needRefresh = true;
      MinMize();
    }
    break;
  case MapHook_AddElecEyeBack:
  case MapHook_AddElecEyeIcon:
    {
      //添加用户电子眼
      m_addElecEyeBtn.MouseUp();
      needRefresh = true;
      AddUserEEyeData();
    }
    break;
  case MapHook_MapAzimuthBack:
  case MapHook_NorthIcon:
  case MapHook_PerspectiveIcon:
  case MapHook_HeadingIcon:
    {
      //地图方向
      m_mapAzimuthBtn.MouseUp();
      needRefresh = true;
      ChangeMapAzimuth();
    }
    break;
  case MapHook_ZoomInBack:
  case MapHook_ZoomInIcon:
    {
      //放大地图
      m_zoomInBtn.MouseUp();
      needRefresh = true;
      if (m_zoomInBtn.IsEnable())
      {
        ZoomIn();
        needRefresh = false;
        m_viewWrapper.Refresh();
      }
    }
    break;
  case MapHook_ZoomOutBack:
  case MapHook_ZoomOutIcon:
    {
      //缩小地图
      m_zoomOutBtn.MouseUp();
      needRefresh = true;
      if (m_zoomOutBtn.IsEnable())
      {        
        ZoomOut();
        needRefresh = false;
        m_viewWrapper.Refresh();
      }
    }
    break;
  case MapHook_ScaleBack:
  case MapHook_ScaleLabel:
  case MapHook_ScaleIcon:
    {
      //比例尺
      m_scaleBtn.MouseUp();
      needRefresh = true;
      CItemSelectHook* itemSelectHook = (CItemSelectHook*)m_viewWrapper.GetHook(DHT_ItemSelectHook);
      if (itemSelectHook)
      {
        itemSelectHook->SetSelectEvent(this, &OnShortcutScaleSelect, m_scaleItemList);
        //窗口探在左边
        itemSelectHook->SetDlgCoordinateDefault(CItemSelectHook::DFT_Coordinate2);
        TurnTo(DHT_ItemSelectHook, false);
      }
    }
    break;
  case MapHook_SoundBack:
  case MapHook_SoundIcon:
    {
      m_soundBtn.MouseUp();
      needRefresh = true;
      //声音设置
      TurnTo(DHT_SoundMenuHook, false);
    }
    break;
  case MapHook_GPSBack:
  case MapHook_GPSIcon:
    {
      m_GPSBtn.MouseUp();
      needRefresh = true;
      //GPS界面
      TurnTo(DHT_GPSHook);
    }
    break;
  case MapHook_ScreenMoadlBack:
  case MapHook_SingleScreenIcon:
  case MapHook_DoubleScreenIcon:
    {
      m_screenMoadlBtn.MouseUp();
      if (m_screenMoadlBtn.IsEnable())
      {
        needRefresh = true;
        //鹰眼图、后续路口选择
        if (SM_None != m_screenMode)
        {
          SetScreenMode(SM_None);
        }
        else
        {
          CItemSelectHook* itemSelectHook = (CItemSelectHook*)m_viewWrapper.GetHook(DHT_ItemSelectHook);
          if (itemSelectHook)
          {
            itemSelectHook->SetSelectEvent(this, &OnSrcModalSelect, m_srcModalItemList);
            //窗口弹在中间
            itemSelectHook->SetDlgCoordinateDefault(CItemSelectHook::DFT_Coordinate1);
            TurnTo(DHT_ItemSelectHook, false);
          }
        } 
      }
    }
    break;
  case MapHook_SetStartBack:
  case MapHook_SetStartIcon:
  case MapHook_SetStartLabel:
    {
      m_setStartBtn.MouseUp();
      needRefresh = true;
      if (m_setStartBtn.IsEnable())
      {
        SetRouteStart();
        needRefresh = false;
        m_viewWrapper.Refresh();
      }
    }
    break;
  case MapHook_SetEndBack:
  case MapHook_SetEndIcon:
  case MapHook_SetEndLabel:
    {
      m_setEndBtn.MouseUp();
      needRefresh = true;
      if (m_setEndBtn.IsEnable())
      {
        unsigned int rt = SetRouteEnd();
        if (UeRoute::PEC_Success == rt)
        {
          RoutePlan(Plan_Multiple);
        }
        needRefresh = false;
        m_viewWrapper.Refresh();
      }
    }
    break;
  case MapHook_SetWayBack:
  case MapHook_SetWayIcon:
  case MapHook_SetWayLabel:
    {
      m_setWayBtn.MouseUp();
      needRefresh = true;
      if (m_setWayBtn.IsEnable())
      {        
        SetRouteMiddle();
        needRefresh = false;
        m_viewWrapper.Refresh();
      }
      m_viewWrapper.Refresh();
    }
    break;
  case MapHook_FixedPostionBack:
  case MapHook_FixedPostionIcon:
  case MapHook_FixedPostionLabel:
    {
      //定位
      m_fixedPostionBtn.MouseUp();
      needRefresh = true;
      MoveToGPS();
    }
    break;
  case MapHook_DetailBack1:
  case MapHook_DetailIcon1:
  case MapHook_DetailLabe1:
    {
      //详情
      m_detailBtn1.MouseUp();
      needRefresh = true;
      OpenDetailHook();
    }
    break;
  case MapHook_DetailBack2:
  case MapHook_DetailIcon2:
  case MapHook_DetailLabe2:
    {
      m_detailBtn2.MouseUp();
      needRefresh = true;
      OpenDetailHook();
    }
    break;
  case MapHook_GuideInfoCenterBack:
  case MapHook_GuideInfoCenterIcon:
  case MapHook_GuideInfoCenterLabe:
    {
      m_guideInfoCenterBtn.MouseUp();
      needRefresh = true;
      OpenDetailHook();
    }
    break;
  }
  //如果鼠标点击的不是地图则重新开始计时
  if (CT_Unknown != m_downElementType)
  {
    RestarGuiTimer();  
  }
  if (needRefresh)
  {                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       
    Refresh();
  }
  return ctrlType;
}

void UeGui::CMapHook::HideAllCtrl()
{
  ShowMinimizeBtn(false);
  ShowAddElecEyeBtn(false);
  ShowMapAzimuthBtn(false);
  ShowMapScalingBtn(false);
  ShowSetDestPointBtn(false);
  ShowFixedPostionBtn(false);
  ShowDetailBtn1(false);
  ShowDetailBtn2(false);
  ShowGuideInfoBtn(false);
  ShowElecEye(false);
}

void UeGui::CMapHook::ShowMinimizeBtn( bool show /*= true*/ )
{
  m_miniMizeBtn.SetVisible(show);
  m_soundBtn.SetVisible(show);
  m_GPSBtn.SetVisible(show);
  m_screenMoadlBtn.SetVisible(show);
  ResetLanPos();
}

void UeGui::CMapHook::ShowAddElecEyeBtn( bool show /*= true*/ )
{
  m_addElecEyeBtn.SetVisible(show);
}

void UeGui::CMapHook::ShowMapAzimuthBtn( bool show /*= true*/ )
{
  m_mapAzimuthBtn.SetVisible(show);
}

void UeGui::CMapHook::ShowMapScalingBtn( bool show /*= true*/ )
{
  m_zoomInBtn.SetVisible(show);
  m_zoomOutBtn.SetVisible(show);
  m_scaleBtn.SetVisible(show);
}

void UeGui::CMapHook::ShowSetDestPointBtn( bool show /*= true*/ )
{
  m_setStartBtn.SetVisible(show);
  m_setWayBtn.SetVisible(show);
  m_setEndBtn.SetVisible(show);
  if (show)
  {
    RefreshSetPointStatus();
  }
}

void UeGui::CMapHook::ShowFixedPostionBtn( bool show /*= true*/ )
{
  m_fixedPostionBtn.SetVisible(show);
}

void UeGui::CMapHook::ShowDetailBtn1( bool show /*= true*/ )
{
  m_detailBtn1.SetVisible(show);
}

void UeGui::CMapHook::ShowDetailBtn2( bool show /*= true*/ )
{
  m_detailBtn2.SetVisible(show);
}

void UeGui::CMapHook::ShowGuideInfoBtn( bool show /*= true*/ )
{
  m_guideInfoLeftBtn.SetVisible(show);
  m_guideInfoCenterBtn.SetVisible(show);
  m_guideInfoRightBtn.SetVisible(show);
  m_delimiter1.SetVisible(show);
  m_delimiter2.SetVisible(show);
}

void UeGui::CMapHook::ShowCompass( bool show /*= true*/ )
{
  m_bIsCompassShown = show;
}

void UeGui::CMapHook::ShowElecEye( bool show /*= true*/ )
{
  m_elecEye.SetVisible(show);
}

void UeGui::CMapHook::ShowTimeBtn( bool show /*= true*/ )
{
  m_timerBtn.SetVisible(show);
}

void UeGui::CMapHook::OpenFunctionMenu()
{
  //功能菜单
  CViewHook* viewHook = m_viewWrapper.GetHook(DHT_TypeNoDistQueryListHook);
  if (viewHook)
  {
    viewHook->Init();
  }  
  TurnTo(CViewHook::DHT_MainMenuHook);
}

void UeGui::CMapHook::OpenShortCutMenu()
{
  //快捷菜单
  CViewHook* viewHook = m_viewWrapper.GetHook(DHT_TypeNoDistQueryListHook);
  if (viewHook)
  {
    viewHook->Init();
  }  
  TurnTo(CViewHook::DHT_FastOperationHook);
}

void UeGui::CMapHook::OpenAroundSearchMenu()
{
  //周边
  CViewHook* viewHook = m_viewWrapper.GetHook(DHT_TypeNoDistQueryListHook);
  if (viewHook)
  {
    viewHook->Init();
  }
  TurnTo(CViewHook::DHT_RoundSelectionHook);
}

void UeGui::CMapHook::OpenSearchMenu()
{
  //搜索
  CViewHook* viewHook = m_viewWrapper.GetHook(DHT_TypeNoDistQueryListHook);
  if (viewHook)
  {
    viewHook->Init();
  }  
  CInputSwitchHook *inputHook = (CInputSwitchHook *)m_viewWrapper.GetHook(DHT_InputSwitchHook);
  if (inputHook)
  {
    TurnTo(inputHook->GetCurInputHookType());
  }
}

void UeGui::CMapHook::OpenDetailHook()
{
  // 设置信息
  CGeoPoint<long> pickPos;
  m_viewWrapper.GetPickPos(pickPos);

  DetailInfo detailInfo = {};
  m_viewWrapper.GetPickName(detailInfo.m_name);
  if(::strlen(detailInfo.m_name) > 0)
  {
    detailInfo.m_name[::strlen(detailInfo.m_name) - 1] = '\0';
  } 
  else 
  {
    CGeoPoint<long> pickPos;    
    m_viewWrapper.GetPickPos(pickPos);
    IGui* gui = IGui::GetGui();
    if (!gui->GetDistrictName(pickPos, detailInfo.m_name))
    {
      ::memset(detailInfo.m_name, 0, 256);
    }
  }
  detailInfo.m_position.m_x = pickPos.m_x;
  detailInfo.m_position.m_y = pickPos.m_y;
  CQueryWrapper& queryWrapper = CQueryWrapper::Get();
  const SQLRecord *record = queryWrapper.GetNearestPoi(pickPos);
  if (record)
  {
    if (record->m_pchTelStr)
    {
      ::strcpy((char*)detailInfo.m_telephone, record->m_pchTelStr);
    }
    if (record->m_pchAddrStr)
    {
      ::strcpy((char*)detailInfo.m_address, record->m_pchAddrStr);
    }
  }

  //如果没有具体地址，则显示省、市、区
  if(detailInfo.m_address[0] == 0)
  {
    IGui* gui = IGui::GetGui();
    if (!gui->GetDistrictName(pickPos, detailInfo.m_address))
    {
      ::memset(detailInfo.m_name, 0, 256);
    }
  }

  CDetailMessageHook* detailMessageHook = (CDetailMessageHook*)m_viewWrapper.GetHook(CViewHook::DHT_DetailMessageHook);
  if (detailMessageHook)
  {
    detailMessageHook->SetDetailInfoData(detailInfo);
  }
  //打开详情界面
  TurnTo(CViewHook::DHT_DetailMessageHook);
}

void UeGui::CMapHook::ChangeMapAzimuth()
{
  m_viewWrapper.NextState();
  RefreshMapAzimuthIcon();
}

void UeGui::CMapHook::ZoomIn()
{
  m_viewWrapper.ZoomIn(1, 0);
  RefreshZoomingInfo();
}

void UeGui::CMapHook::ZoomOut()
{
  m_viewWrapper.ZoomOut(1, 0);
  RefreshZoomingInfo();
}

void UeGui::CMapHook::MinMize()
{
  m_viewWrapper.HideWindow();
}

unsigned int UeGui::CMapHook::SetRouteStart()
{
  return m_routeWrapper.SetRouteStart();
}

unsigned int UeGui::CMapHook::SetRouteMiddle()
{
  unsigned int rt = m_routeWrapper.SetRouteMiddle();
  if (UeRoute::PEC_Success == rt)
  {
    CMessageDialogEvent dialogEvent(this, DHT_MapHook, NULL);
    CMessageDialogHook::ShowMessageDialog(MB_NONE, "规划中，请稍候...", dialogEvent);
    rt = m_routeWrapper.RoutePlan();
    if (UeRoute::PEC_Success == rt)
    {
      Sleep(1000);
      CMessageDialogHook::CloseMessageDialog();
      //切换到路径规划菜单
      SwitchingGUI(GUI_RouteCalculate);
    }
    else
    {
      m_routeWrapper.RemovePosition();
      CMessageDialogHook::ShowMessageDialog(MB_NONE, "路径规划失败", dialogEvent);
      Sleep(2000);
      CMessageDialogHook::CloseMessageDialog();
    } 
  }
  return rt;
}

unsigned int UeGui::CMapHook::SetRouteEnd()
{  
  unsigned int rt = m_routeWrapper.SetRouteEnd();
  if (UeRoute::PEC_TooShortest == rt)
  {
    CMessageDialogEvent dialogEvent(this, DHT_MapHook, NULL);
    CMessageDialogHook::ShowMessageDialog(MB_NONE, "起点和目的地距离太近", dialogEvent);
    Sleep(1000);
    CMessageDialogHook::CloseMessageDialog();
  }
  return rt;
}

unsigned int UeGui::CMapHook::DoRoutePlan( PlanType planType )
{
  CMessageDialogEvent dialogEvent(this, DHT_MapHook, NULL);
  CMessageDialogHook::ShowMessageDialog(MB_NONE, "规划中，请稍候...", dialogEvent);
  unsigned int rt = UeRoute::PEC_Success;
  if (Plan_Multiple == planType)
  {
    m_planType = Plan_Multiple;
    rt = m_routeWrapper.RoutePlan();
    if (UeRoute::PEC_Success == rt)
    {
      rt = m_routeWrapper.MultiRoutePlan();
    }    
  }
  else
  {
    rt = m_routeWrapper.RoutePlan();
    m_planType = Plan_Single;
  }
  if (UeRoute::PEC_Success != rt)
  {
    //删除所有经由点
    m_routeWrapper.RemovePosition();
    if (UeRoute::PEC_TooShortest == rt)
    {
      CMessageDialogHook::ShowMessageDialog(MB_NONE, "起点和目的地距离太近，请检查.", dialogEvent);
    }
    else
    {
      CMessageDialogHook::ShowMessageDialog(MB_NONE, "路径规划失败.", dialogEvent);
    }        
    Sleep(500);
  }
  CMessageDialogHook::CloseMessageDialog();
  return rt;
}

void UeGui::CMapHook::DoStopGuide()
{
  //初始化电子眼状态
  m_elecEyeStatus = false;
  //设置视图模式
  m_viewWrapper.SetViewOpeMode(VM_Guidance);
  //切换到浏览菜单
  SwitchingGUI(GUI_MapBrowse);
  //更新菜单
  UpdateMenu();
  //更新底部状态栏
  UpdateGuideInfo(NULL, 0, -1);
  //刷新底部状态栏
  RefreshLocationInfo();
  //关闭鹰眼图
  m_viewWrapper.ShowEagleView(false);
  //关闭后续路口
  m_mapGuideInfoView.SetIsShowRouteGuideList(false);
  //关闭高速看板
  m_mapGuideInfoView.SetIsShowHightSpeedBoard(false);
}

unsigned int UeGui::CMapHook::RoutePlan( PlanType planType /*= Plan_Single*/ )
{
  unsigned int rt = DoRoutePlan(planType);
  if (UeRoute::PEC_Success == rt)
  {
    m_viewWrapper.AutoScallingMap();
    //刷新比例尺
    RefreshZoomingInfo();
    //切换到路径规划菜单
    SwitchingGUI(GUI_RouteCalculate);
    //保存历史出发地
    m_userDataWrapper.AddHistoryPoint(UeRoute::PT_Start);
    //保存历史目的地
    m_userDataWrapper.AddHistoryPoint(UeRoute::PT_End);
    //保存历史路线
    m_userDataWrapper.AddRecent();
  }
  return rt;
}

unsigned int UeGui::CMapHook::RoutePlan_StartGuidance( PlanType planType /*= Plan_Single*/ )
{
  unsigned int rt = DoRoutePlan(planType);
  if (UeRoute::PEC_Success == rt)
  {
    rt = StartGuidance();
  }
  return rt;
}

unsigned int UeGui::CMapHook::RoutePlan_StartDemo( PlanType planType /*= Plan_Single*/ )
{
  unsigned int rt = DoRoutePlan(planType);
  if (UeRoute::PEC_Success == rt)
  {
    rt = StartDemo();
  }
  return rt;
}

unsigned int UeGui::CMapHook::BackTrackingPlan()
{
  CMessageDialogEvent dialogEvent(this, m_curHookType, NULL);
  CMessageDialogHook::ShowMessageDialog(MB_NONE, "规划中，请稍候...", dialogEvent);
  unsigned int rt = m_routeWrapper.BackTrackingPlan();
  if (UeRoute::PEC_Success == rt)
  {
    m_viewWrapper.AutoScallingMap();
    //刷新比例尺
    RefreshZoomingInfo();
    //切换到路径规划菜单
    SwitchingGUI(GUI_RouteCalculate);
    //规划成功自动跳转到地图界面，并开始导航
    CMessageDialogHook::CloseMessageDialog(DHT_MapHook);
    m_viewWrapper.Refresh();
  }
  else
  {
    m_routeWrapper.RemovePosition();
    CMessageDialogHook::ShowMessageDialog(MB_NONE, "路径规划失败", dialogEvent);
    Sleep(500);
    CMessageDialogHook::CloseMessageDialog();
  }
  return rt;
}

unsigned char UeGui::CMapHook::GetMultiMethodType()
{
  UeRoute::MethodType methType = MT_Max;
  if (Plan_Single == m_planType)
  {
    methType = MT_Max;
  }
  else
  {
    if (GUI_RouteCalculate == m_curGuiType)
    {
      unsigned int planMethod =  m_routeWrapper.GetMethod();
      if (planMethod & UeRoute::RW_Optimal)
      {
        methType = MT_Optimal;
      }
      else if (planMethod & UeRoute::RW_Short)
      {
        methType = MT_Short;
      }
      else if (planMethod & UeRoute::RW_Fast)
      {
        methType = MT_Fast;
      }
      else if (planMethod & UeRoute::RW_Economic)
      {
        methType = MT_Economic;
      }
    }
    else
    {
      methType = MT_Max;
    }
  }
  return methType;
}

unsigned int UeGui::CMapHook::StartDemo( short speed /*= DEFAULT_DEMOSPEED*/ )
{
  unsigned int rt = m_routeWrapper.StartDemo(speed);
  if (UeRoute::PEC_Success == rt)
  {
    m_viewWrapper.SwitchTo(SCALE_100M, 0);
    m_viewWrapper.SetViewOpeMode(VM_Guidance);
    //初始化模拟导航信息
    m_mapSimulationMenu.ReseSimulation();
    //切换到路径规划菜单
    SwitchingGUI(GUI_DemoGuidance);
    //刷新比例尺
    RefreshZoomingInfo();
    //刷新界面
    m_viewWrapper.Refresh();
  }
  else
  {
    CMessageDialogEvent dialogEvent(this, DHT_MapHook, NULL);
    CMessageDialogHook::ShowMessageDialog(MB_NONE, "模拟导航失败", dialogEvent);
    Sleep(500);
    CMessageDialogHook::CloseMessageDialog();
  }
  return rt;
}

unsigned int UeGui::CMapHook::StopDemo()
{
  unsigned int rt = m_routeWrapper.StopDemo();
  return rt;
}

unsigned int UeGui::CMapHook::StartGuidance()
{
  unsigned int rt = m_routeWrapper.StartGuidance();
  if (UeRoute::PEC_Success == rt)
  {
    m_viewWrapper.SwitchTo(SCALE_100M, 0);
    m_viewWrapper.SetViewOpeMode(VM_Guidance);
    m_viewWrapper.MoveToStart();
    //切换到路径规划菜单
    SwitchingGUI(GUI_RealGuidance);
    UpdateMenu();
    //刷新比例尺
    RefreshZoomingInfo();
    //刷新界面
    m_viewWrapper.Refresh();
  }
  else
  {
    CMessageDialogEvent dialogEvent(this, DHT_MapHook, NULL);
    CMessageDialogHook::ShowMessageDialog(MB_NONE, "开始导航失败", dialogEvent);
    Sleep(500);
    CMessageDialogHook::CloseMessageDialog();
  }
  return rt;
}

unsigned int UeGui::CMapHook::StopGuidance()
{
  unsigned int rt = m_routeWrapper.StopGuidance();
  return rt;
}

unsigned int UeGui::CMapHook::EraseRoute()
{
  //删除路线
  m_routeWrapper.EraseRoute();
  //设置视图模式
  m_viewWrapper.SetViewOpeMode(VM_Guidance);
  //切换到浏览状态状态菜单
  SwitchingGUI(GUI_MapBrowse);
  UpdateMenu();
  return UeRoute::PEC_Success;
}

void UeGui::CMapHook::Cancel()
{
  //设置视图模式
  m_viewWrapper.SetViewOpeMode(VM_Guidance);
  //切换到浏览状态状态菜单
  SwitchingGUI(GUI_MapBrowse);
}

void UeGui::CMapHook::Select()
{
  if (m_selectPointObj && m_selectPointEvent)
  {
    CGeoPoint<long> pickPos;
    m_viewWrapper.GetPickPos(pickPos);
    SQLRecord record;
    record.m_x = pickPos.m_x;
    record.m_y = pickPos.m_y;

    // 拾取信息
    char pickName[256];
    m_viewWrapper.GetPickName(pickName);
    if(::strlen(pickName) > 0)
    {
      pickName[::strlen(pickName) - 1] = '\0';
    } 
    else 
    {
      CGeoPoint<long> pickPos;    
      m_viewWrapper.GetPickPos(pickPos);
      IGui* gui = IGui::GetGui();
      if (!gui->GetDistrictName(pickPos, pickName))
      {
        ::memset(pickName, 0, 256);
      }
    }
    record.m_uniName = pickName;
    (*m_selectPointEvent)(m_selectPointObj, &record);
  }
}

void UeGui::CMapHook::SetPickPos( const CGeoPoint<long> &point, const char* name )
{
  RefreshLocationInfo(name);  
  CGeoPoint<short> scrPoint;
  m_viewWrapper.SetPickPos(point, scrPoint, true);
}

void UeGui::CMapHook::SetPickPos( PointList pointList, unsigned short posIndex )
{
  m_viewWrapper.SwitchTo(SCALE_200M, 0);
  RefreshZoomingInfo();
  ClearQueryPointList();
  if ((posIndex >= 0) && (posIndex < pointList.size()))
  {
    m_queryPointIndex = posIndex;    
    m_queryPointList = pointList;
    m_curPoint = pointList[posIndex];
    SetPickPos(m_curPoint.m_point, m_curPoint.m_name);    
  }
  SwitchingGUI(GUI_QueryPoint);
}

void UeGui::CMapHook::SelectPoint( const CGeoPoint<long> &point, const char* name, void* selectPointObj, SelectPointEvent selectEvent )
{
  m_selectPointObj = selectPointObj;
  m_selectPointEvent = selectEvent;
  m_viewWrapper.SwitchTo(SCALE_200M, 0);  
  RefreshZoomingInfo();
  SetPickPos(point, name);
  SwitchingGUI(GUI_SelectPoint);  
}

void UeGui::CMapHook::OverviewRoute()
{
  SwitchingGUI(GUI_OverviewRoute);
  m_viewWrapper.AutoScallingMap(true);
}

void UeGui::CMapHook::RefreshLocationInfo( const char* name /*= NULL*/ )
{
  if (NULL == name)
  {
    // 拾取信息
    char pickName[256];
    m_viewWrapper.GetPickName(pickName);
    if(::strlen(pickName) > 0)
    {
      pickName[::strlen(pickName) - 1] = '\0';
    } 
    else 
    {
      CGeoPoint<long> pickPos;    
      m_viewWrapper.GetPickPos(pickPos);
      IGui* gui = IGui::GetGui();
      if (!gui->GetDistrictName(pickPos, pickName))
      {
        ::memset(pickName, 0, 256);
      }
    }
    m_detailBtn1.SetCaption(pickName);
    m_detailBtn2.SetCaption(pickName);
    m_guideInfoCenterBtn.SetCaption(pickName);
  }
  else
  {
    m_detailBtn1.SetCaption(name);
    m_detailBtn2.SetCaption(name);
    m_guideInfoCenterBtn.SetCaption(name);
  }
}

void UeGui::CMapHook::RefreshMapAzimuthIcon()
{
  CViewState *curState = m_viewWrapper.GetMainViewState();
  m_mapAzimuthBtn.SetVisible(false);
  if (curState)
  {
    switch (curState->GetType())
    {
    case VT_North:
      {
        m_mapAzimuthBtn.SetIconElement(GetGuiElement(MapHook_NorthIcon));
        m_settingWrapper.SetMapDirection(ViewSettings::MD_DueNorth);
      }
      break;
    case VT_Heading:
      {
        m_mapAzimuthBtn.SetIconElement(GetGuiElement(MapHook_HeadingIcon));
        m_settingWrapper.SetMapDirection(ViewSettings::MD_HeadMap);
      }
      break;
    case VT_Perspective:
      {
        m_mapAzimuthBtn.SetIconElement(GetGuiElement(MapHook_PerspectiveIcon));
        m_settingWrapper.SetMapDirection(ViewSettings::MD_25DMap);
      }
      break;
    }
    m_settingWrapper.SaveNaviationSettings();
  }
  m_mapAzimuthBtn.SetVisible(true);
}

void UeGui::CMapHook::RefreshZoomingInfo()
{
  short curLevel = 0;
  int scale = 0;
  m_viewWrapper.GetScale(curLevel, scale);
  int  maxScaleLevel =  m_viewWrapper.GetMaxScaleLevel();

  if (curLevel <= 1)
  {
    // 设置缩小不可用
    m_zoomInBtn.SetEnable(false);
    if (!m_zoomOutBtn.IsEnable())
    {
      m_zoomOutBtn.SetEnable(true);
    }    
  }
  else if(curLevel >= maxScaleLevel) 
  {
    // 设置放大不可用
    m_zoomOutBtn.SetEnable(false);
    if (!m_zoomInBtn.IsEnable())
    {
      m_zoomInBtn.SetEnable(true);
    }
  }
  else
  {
    if (!m_zoomInBtn.IsEnable())
    {
      m_zoomInBtn.SetEnable(true);
    }
    if (!m_zoomOutBtn.IsEnable())
    {
      m_zoomOutBtn.SetEnable(true);
    }    
  }
  //更新比例尺
  RefreshScaleLabel(curLevel);
}

void UeGui::CMapHook::RefreshScaleLabel( unsigned short scaleLevel )
{
  char msg[10] = {};
  switch (scaleLevel)
  {
  case SCALE_10M:
    {
      strcpy(msg, "25m");
    }
    break;
  case SCALE_25M:
    {
      strcpy(msg, "25m");
    }
    break;
  case SCALE_50M:
    {
      strcpy(msg, "50m");
    }
    break;
  case SCALE_100M:
    {
      strcpy(msg, "100m");
    }
    break;
  case SCALE_200M:
    {
      strcpy(msg, "200m");
    }
    break;
  case SCALE_500M:
    {
      strcpy(msg, "500m");
    }
    break;
  case SCALE_1KM:
    {
      strcpy(msg, "1km");
    }
    break;
  case SCALE_2KM:
    {
      strcpy(msg, "2km");
    }
    break;
  case SCALE_5KM:
    {
      strcpy(msg, "5km");
    }
    break;
  case SCALE_10KM:
    {
      strcpy(msg, "10km");
    }
    break;
  case SCALE_20KM:
    {
      strcpy(msg, "20km");
    }
    break;
  case SCALE_50KM:
    {
      strcpy(msg, "50km");
    }
    break;
  case SCALE_100KM:
    {
      strcpy(msg, "100km");
    }
    break;
  case SCALE_200KM:
    {
      strcpy(msg, "200km");
    }
    break;
  case SCALE_1000KM:
    {
      strcpy(msg, "1000km");
    }
    break;
  default:
    {
      return;
    }    
  }
  GuiElement* guiElement = m_scaleBtn.GetLabelElement();
  if (guiElement)
  {
    if (scaleLevel <= SCALE_50KM)
    {
      guiElement->m_textStyle = guiElement->m_normalTextStylpe;
    }
    else if (scaleLevel <= SCALE_200KM)
    {
      guiElement->m_textStyle = guiElement->m_focusTextStyle;
    }
    else
    {
      guiElement->m_textStyle = guiElement->m_disabledTextStype;
    }
  }
  m_scaleBtn.SetCaption(msg);
}

void UeGui::CMapHook::ShowGPSNum( int locationNum )
{
  if (locationNum > 12)
  {
    locationNum = 12;
  }
  GuiElement* guiElement = NULL;
  switch (locationNum)
  {
  case 1:
    {
      guiElement = GetGuiElement(MapHook_GPSIcon1);
      break;
    }
  case 2:
    {
      guiElement = GetGuiElement(MapHook_GPSIcon2);
      break;
    }
  case 3:
    {
      guiElement = GetGuiElement(MapHook_GPSIcon3);
      break;
    }
  case 4:
    {
      guiElement = GetGuiElement(MapHook_GPSIcon4);
      break;
    }
  case 5:
    {
      guiElement = GetGuiElement(MapHook_GPSIcon5);
      break;
    }
  case 6:
    {
      guiElement = GetGuiElement(MapHook_GPSIcon6);
      break;
    }
  case 7:
    {
      guiElement = GetGuiElement(MapHook_GPSIcon7);
      break;
    }
  case 8:
    {
      guiElement = GetGuiElement(MapHook_GPSIcon8);
      break;
    }
  case 9:
    {
      guiElement = GetGuiElement(MapHook_GPSIcon9);
      break;
    }
  case 10:
    {
      guiElement = GetGuiElement(MapHook_GPSIcon10);
      break;
    }
  case 11:
    {
      guiElement = GetGuiElement(MapHook_GPSIcon11);
      break;
    }
  case 12:
    {
      guiElement = GetGuiElement(MapHook_GPSIcon12);
      break;
    }
  default:
    {
      guiElement = GetGuiElement(MapHook_GPSIcon0);
      break;
    }
  }

  if (guiElement)
  {
    GuiElement* gpsElement = GetGuiElement(MapHook_GPSIcon);
    if (gpsElement)
    {
      gpsElement->m_backStyle = guiElement->m_bkNormal;
    }
  }  
}

void UeGui::CMapHook::RefreshSetPointStatus()
{
  //判断是否设置了终点
  PlanPosition endPos;
  endPos.m_type = UeRoute::PT_End;
  unsigned int rt = m_routeWrapper.GetPosition(endPos);
  if ((UeRoute::PT_Invalid == endPos.m_type) || (!endPos.IsValid()))
  {
    m_setWayBtn.SetEnable(false);
  }
  else
  {
    m_setWayBtn.SetEnable(true);
  }
}

void UeGui::CMapHook::FillGuidanceInfo()
{
  //更新引导菜单状态
  UpdateMenu();
}

void UeGui::CMapHook::UpdateGuideInfo( const char* routeName, double speed, double distance )
{
  if (speed < 0)
  {
    speed = 0;
  }
  //车子行驶速度
  char buf[32] = {};
  ::sprintf(buf, "%0.0f km/h", speed);
  m_guideInfoLeftBtn.SetCaption(buf);
  //当前行驶道路名称
  m_guideInfoCenterBtn.SetCaption(routeName);
  //距离终点距离
  if (distance >= 0)
  {
    if (distance <= 1000)
    {
      ::sprintf(buf, "%dm", static_cast<int>(distance));
    } 
    else
    {
      ::sprintf(buf, "%.1fkm", distance / 1000.0);
    }
    m_guideInfoRightBtn.SetCaption(buf);
  }
  else
  {
    m_guideInfoRightBtn.SetCaption("--:--");
  }
}

void UeGui::CMapHook::MoveToStart()
{
  m_viewWrapper.MoveToStart();
  //根据当前导航状态切换界面菜单
  short planState = m_routeWrapper.GetPlanState();
  if (UeRoute::PS_DemoGuidance == planState)
  {    
    SwitchingGUI(GUI_DemoGuidance);
  }
  if (UeRoute::PS_RealGuidance == planState)
  {
    SwitchingGUI(GUI_RealGuidance);
  }
  else
  {
    SwitchingGUI(GUI_MapBrowse);
  }
  //收缩菜单
  UpdateMenu(MUT_Close);
}

void UeGui::CMapHook::MoveToCar()
{
  m_viewWrapper.MoveToCar();
  //根据当前导航状态切换界面菜单
  short planState = m_routeWrapper.GetPlanState();
  if (UeRoute::PS_DemoGuidance == planState)
  {    
    SwitchingGUI(GUI_DemoGuidance);
  }
  if (UeRoute::PS_RealGuidance == planState)
  {
    SwitchingGUI(GUI_RealGuidance);
  }
  else
  {
    SwitchingGUI(GUI_MapBrowse);
  }
  //收缩菜单
  UpdateMenu(MUT_Close);
}

void UeGui::CMapHook::MoveToGPS()
{
  m_viewWrapper.MoveToGPS();
  //根据当前导航状态切换界面菜单
  short planState = m_routeWrapper.GetPlanState();
  if (UeRoute::PS_DemoGuidance == planState)
  {    
    SwitchingGUI(GUI_DemoGuidance);
  }
  if (UeRoute::PS_RealGuidance == planState)
  {
    SwitchingGUI(GUI_RealGuidance);
  }
  else
  {
    SwitchingGUI(GUI_MapBrowse);
  }
  //收缩菜单
  UpdateMenu(MUT_Close);
  //刷新详情信息
  RefreshLocationInfo();
}

void UeGui::CMapHook::ClearQueryPointList()
{
  ::memset(&m_curPoint, 0, sizeof(PointInfo));
  m_queryPointIndex = -1;
  m_queryPointList.clear();
}

void UeGui::CMapHook::SetQueryPos( PosType posType )
{
  if (m_queryPointList.empty())
  {
    return;
  }
  if ((m_queryPointIndex >= 0) && (m_queryPointIndex <= m_queryPointList.size() - 1))
  {
    switch (posType)
    {
    case PT_Current:
      {
        //定位到当前条不需要处理
        break;
      }
    case PT_Previous:
      {
        if (m_queryPointIndex > 0)
        {
          m_queryPointIndex--;
          m_curPoint = m_queryPointList[m_queryPointIndex];
        }
        break;
      }
    case PT_Next:
      {
        if (m_queryPointIndex < m_queryPointList.size() - 1)
        {
          m_queryPointIndex++;
          m_curPoint = m_queryPointList[m_queryPointIndex];
        }
        break;
      }
    }
    if (m_curPoint.m_point.IsValid())
    {
      SetPickPos(m_curPoint.m_point, m_curPoint.m_name);
    } 
  }
}

bool UeGui::CMapHook::HaveNextQueryPoint( PosType posType )
{
  if (m_queryPointList.empty())
  {
    return false;
  }
  bool haveNext = false;
  if ((m_queryPointIndex >= 0) && (m_queryPointIndex <= m_queryPointList.size() - 1))
  {
    switch (posType)
    {
    case PT_Previous:
      {
        if (m_queryPointIndex <= 0)
        {
          haveNext = false;
        }
        else
        {
          haveNext = true;
        }
        break;
      }
    case PT_Next:
      {
        if (m_queryPointIndex >= m_queryPointList.size() - 1)
        {
          haveNext = false;
        }
        else
        {
          haveNext = true;
        }
        break;
      }
    }
  }
  return haveNext;
}

void UeGui::CMapHook::RestarGuiTimer()
{
  m_guiTimerInterval = TIMER_INTERVAL_6S;
}

void UeGui::CMapHook::CloseGuiTimer()
{
  m_guiTimerInterval = 0;
}

bool UeGui::CMapHook::IsGuiTimerDown()
{
  return 0 == m_guiTimerInterval;
}

void UeGui::CMapHook::ShowGuideView()
{
  UpdateMenu();
  m_viewWrapper.ShowGuidanceView();
}

bool UeGui::CMapHook::IsShowCompass()
{
  bool isShowCompass = m_settingWrapper.GetCompassPrompt();
  return isShowCompass && m_bIsCompassShown;
  //测试
  //return m_bIsCompassShown;
}

bool UeGui::CMapHook::HaveElecEyePrompt()
{
  return m_elecEyeStatus;
}

bool UeGui::CMapHook::IsSplitScreen()
{
  //读取当前规划状态
  short planState = m_routeWrapper.GetPlanState();
  if ((UeRoute::PS_DemoGuidance == planState) || (UeRoute::PS_RealGuidance == planState))
  {
    if ((m_viewWrapper.IsGuidanceViewShown()) || (SM_EagelView == m_screenMode) || (SM_DoubleScreen == m_screenMode))
    {
      return true;
    }
  }
  return false;
}

void UeGui::CMapHook::UpdateElecEyeInfo()
{
  //显示电子眼
  EEyeProp eyeProp;
  double distance = 0.0;
  bool rt = m_routeWrapper.GetCurElecEye(eyeProp, distance);
  if (rt && (distance > 0))
  {
    //更新电子眼状态
    m_elecEyeStatus = true;
    //读取最大提示距离
    if (distance > m_elecMaxPromptDist)
    {
      m_elecMaxPromptDist = distance;
    }    
    UpdateElecIcon(eyeProp);
    UpdateElecProgress(distance);
  }
  else
  {
    m_elecMaxPromptDist = 0;
    m_elecEyeStatus = false;
    ShowElecEye(false);
  }
}

void UeGui::CMapHook::UpdateElecProgress( double distance /*= 0*/ )
{
  int promptDist = m_elecMaxPromptDist - distance;
  if (promptDist < 0)
  {
    promptDist = m_elecMaxPromptDist;
  }
  GuiElement* guiElement = m_elecEye.GetLabelElement();
  if (guiElement)
  {
    int progress = 0;
    if (m_elecMaxPromptDist > 0)
    {
      progress = (m_elecProgressBarWidth * promptDist) / m_elecMaxPromptDist;
      if (progress <= 0)
      {
        progress = 1;
      }      
    }
    guiElement->m_width = progress;
  }
}

bool UeGui::CMapHook::UpdateElecIcon( UeRoute::EEyeProp& eyeProp )
{
  short promptElementType = MapHook_Begin;
  switch (eyeProp.m_type)
  {
  case UeRoute::TVT_TrafficLights:
    {
      //红绿灯
      promptElementType = MapHook_ElecEyeIconType_13;
      break;
    }
  case UeRoute::TVT_SpeedLimit:
    {
      //限速
      if ((eyeProp.m_speed >= 20) && (eyeProp.m_speed <= 25))
      {
        promptElementType = MapHook_ElecEyeIconType_1;
      }
      else if ((eyeProp.m_speed > 25) && (eyeProp.m_speed <= 35))
      {
        promptElementType = MapHook_ElecEyeIconType_2;
      }
      else if ((eyeProp.m_speed > 35) && (eyeProp.m_speed <= 45))
      {
        promptElementType = MapHook_ElecEyeIconType_3;
      }
      else if ((eyeProp.m_speed > 45) && (eyeProp.m_speed <= 55))
      {
        promptElementType = MapHook_ElecEyeIconType_4;
      }
      else if ((eyeProp.m_speed > 55) && (eyeProp.m_speed <= 65))
      {
        promptElementType = MapHook_ElecEyeIconType_5;
      }
      else if ((eyeProp.m_speed > 65) && (eyeProp.m_speed <= 75))
      {
        promptElementType = MapHook_ElecEyeIconType_6;
      }
      else if ((eyeProp.m_speed > 75) && (eyeProp.m_speed <= 85))
      {
        promptElementType = MapHook_ElecEyeIconType_7;
      }
      else if ((eyeProp.m_speed > 85) && (eyeProp.m_speed <= 95))
      {
        promptElementType = MapHook_ElecEyeIconType_8;
      }
      else if ((eyeProp.m_speed > 95) && (eyeProp.m_speed <= 105))
      {
        promptElementType = MapHook_ElecEyeIconType_9;
      }
      else if ((eyeProp.m_speed > 105) && (eyeProp.m_speed <= 115))
      {
        promptElementType = MapHook_ElecEyeIconType_10;
      }
      else if ((eyeProp.m_speed > 105) && (eyeProp.m_speed <= 115))
      {
        promptElementType = MapHook_ElecEyeIconType_11;
      }
      else
      {
        promptElementType = MapHook_ElecEyeIconType_12;
      }
      break;
    }
  case UeRoute::TVT_NormalCamera:
  case UeRoute::TVT_InTunnel:
  case UeRoute::TVT_TunnelPort:
    {
      //电子眼
      promptElementType = MapHook_ElecEyeIconType_12;
      break;
    }
  }
  if (MapHook_Begin != promptElementType)
  {
    ChangeElementIcon(m_elecEye.GetIconElement(), GetGuiElement(promptElementType));
    return true;
  }
  return false;
}

void UeGui::CMapHook::SetScreenMode(ScreenMode screenMode)
{
  if (SM_None != screenMode)
  {
    //打开
    m_screenMode = screenMode;
    switch (m_screenMode)
    {
    case SM_DoubleScreen:
      {
        //打开双屏
        break;
      }
    case SM_EagelView:
      {
        //打开鹰眼图
        m_viewWrapper.ShowEagleView();
        break;
      }
    case SM_RouteGuidance:
      {
        //打开后续路口
        m_mapGuideInfoView.SetIsShowRouteGuideList(true);
        m_mapGuideInfoView.Update(MUT_Normal);
        break;
      }
    case SM_HighWayBoard:
      {
        //打开高速看板
        m_mapGuideInfoView.SetIsShowHightSpeedBoard(true);
        m_mapGuideInfoView.Update(MUT_Normal);
        break;
      }
    }
  }
  else
  {
    //关闭
    switch (m_screenMode)
    {
    case SM_DoubleScreen:
      {
        //关闭双屏
        break;
      }
    case SM_EagelView:
      {
        //关闭鹰眼图
        m_viewWrapper.ShowEagleView(false);   
        break;
      }
    case SM_RouteGuidance:
      {
        //关闭后续路口
        m_mapGuideInfoView.SetIsShowRouteGuideList(false);
        m_mapGuideInfoView.Update(MUT_Normal);
        break;
      }
    case SM_HighWayBoard:
      {
        //关闭高速看板
        m_mapGuideInfoView.SetIsShowHightSpeedBoard(false);
        m_mapGuideInfoView.Update(MUT_Normal);
        break;
      }
    }
    m_screenMode = SM_None;
  }
  RefreshScreenModeIcon();
}

void UeGui::CMapHook::RefreshScreenModeIcon()
{
  GuiElement* guiElement = NULL;
  m_screenMoadlBtn.SetVisible(false);
  if (SM_None == m_screenMode)
  {
    guiElement = GetGuiElement(MapHook_SingleScreenIcon);
    if (guiElement)
    {
      m_screenMoadlBtn.SetIconElement(guiElement);
    }    
  }
  else
  {
    guiElement = GetGuiElement(MapHook_DoubleScreenIcon);
    if (guiElement)
    {
      m_screenMoadlBtn.SetIconElement(guiElement);
    }    
  }
  m_screenMoadlBtn.SetVisible(true);
}

bool UeGui::CMapHook::ChangeElementIcon( GuiElement* destElement, GuiElement* srcElement )
{
  if (destElement && srcElement)
  {
    destElement->m_backStyle = srcElement->m_backStyle;
    return true;
  }
  return false;
}

void UeGui::CMapHook::OnRestoreRote( CAggHook* sender, ModalResultType modalResult )
{
  if (sender)
  {
    CMapHook* mapHook = dynamic_cast<CMapHook*>(sender);
    if (MR_OK == modalResult)
    {
      mapHook->RestoreRote();
    }
    else if (MR_CANCEL == modalResult)
    {
      mapHook->CancelRestoreRote();
    }
  }
}

void UeGui::CMapHook::RestoreRote()
{
  //恢复路线
  CMessageDialogEvent dialogEvent(this, DHT_MapHook, NULL);
  CMessageDialogHook::ShowMessageDialog(MB_NONE, "上次导航恢复中，请稍候...", dialogEvent);

  m_needRestoreRoute = false;
  bool restoreState = true;
  if (m_restorePoiList.size() >= 2)
  {
    m_routeWrapper.SetMethod(m_restoreRouteType);
    unsigned int rt = UeRoute::PEC_Success;
    for (int i = 0; i < m_restorePoiList.size(); ++i)
    {
      UeRoute::PlanPosition& planPosition = m_restorePoiList[i];
      rt = m_routeWrapper.SetPosition(planPosition);
      if (UeRoute::PEC_Success != rt)
      {
        restoreState = false;
      }
    }
    if (restoreState)
    {
      unsigned int rt = UeRoute::PEC_Success;
      m_planType = Plan_Multiple;
      rt = m_routeWrapper.RoutePlan();
      if (UeRoute::PEC_Success == rt)
      {
        rt = m_routeWrapper.MultiRoutePlan();
      }
      if (UeRoute::PEC_Success != rt)
      {
        restoreState = false;
      }
    }
  }

  if (restoreState)
  {
    CMessageDialogHook::CloseMessageDialog();
    StartGuidance();
  }
  else
  {
    m_routeWrapper.RemovePosition();
    CMessageDialogHook::ShowMessageDialog(MB_NONE, "路径恢复失败", dialogEvent);
    Sleep(500);
    CMessageDialogHook::CloseMessageDialog();
  }
}

void UeGui::CMapHook::CancelRestoreRote()
{
  m_needRestoreRoute = false;
  m_userDataWrapper.ClearLastRoute();
  m_restorePoiList.clear();
}

void UeGui::CMapHook::ResetLanPos()
{
  //设置车道显示位置
  unsigned short planState = m_routeWrapper.GetPlanState();
  if ((UeRoute::PS_DemoGuidance == planState) || (UeRoute::PS_RealGuidance == planState))
  {
    GuiElement* guiElement = NULL;
    CGeoPoint<short> scrPoint;
    if (m_miniMizeBtn.IsVisible())
    {
      guiElement = m_addElecEyeBtn.GetCenterElement();
      if (guiElement)
      {      
        scrPoint.m_x = guiElement->m_startX + guiElement->m_width + guiElement->m_width * 2 / 10;
        scrPoint.m_y = guiElement->m_startY + m_lanHight;      
      }
    }
    else
    {
      guiElement = m_miniMizeBtn.GetCenterElement();
      if (guiElement)
      {      
        scrPoint.m_x = guiElement->m_startX + guiElement->m_width + guiElement->m_width * 2 / 10;
        scrPoint.m_y = guiElement->m_startY + m_lanHight + 10;      
      }
    }
    m_viewWrapper.SetLanePos(scrPoint, m_lanWidth, m_lanHight);
  }
}

void UeGui::CMapHook::SrcModalSelect( short selectIndex )
{
  SetScreenMode((ScreenMode)selectIndex);
}

void UeGui::CMapHook::OnSrcModalSelect( CAggHook* sender, short selectIndex )
{
  if (sender)
  {
    CMapHook* mapHook = dynamic_cast<CMapHook*>(sender);
    mapHook->SrcModalSelect(selectIndex);
  }
}

void UeGui::CMapHook::ShortcutScaleSelect( short selectIndex )
{
  switch (selectIndex)
  {
  case 0:
    {
      m_viewWrapper.SwitchTo(SCALE_100M, 0);
      break;
    }
  case 1:
    {
      m_viewWrapper.SwitchTo(SCALE_2KM, 0);
      break;
    }
  case 2:
    {
      m_viewWrapper.SwitchTo(SCALE_50KM, 0);
      break;
    }
  case 3:
    {
      m_viewWrapper.SwitchTo(SCALE_1000KM, 0);
      break;
    }
  }
  RefreshZoomingInfo();
  m_viewWrapper.Refresh();
}

void UeGui::CMapHook::OnShortcutScaleSelect( CAggHook* sender, short selectIndex )
{
  if (sender)
  {
    CMapHook* mapHook = dynamic_cast<CMapHook*>(sender);
    mapHook->ShortcutScaleSelect(selectIndex);
  }
}

void UeGui::CMapHook::AddUserEEyeData()
{
  bool addStatus = false;
  if (m_net)
  {
    CGeoPoint<long> pickPos; 
    ViewOpeMode viewMode = m_viewWrapper.GetViewOpeMode();
    if (VM_Browse == viewMode)
    {
      m_viewWrapper.GetPickPos(pickPos);
    }
    else
    {
      const UeMap::GpsCar gpsCar = m_viewWrapper.GetGpsCar();
      pickPos.m_x = gpsCar.m_curPos.m_x;
      pickPos.m_y = gpsCar.m_curPos.m_y;    
    }

    //用户电子眼数据结构
    UserEEyeEntryData eEyeEntryData;
    //读取区域名作为地址
    IGui* gui = IGui::GetGui();
    if (!gui->GetDistrictName(pickPos, (char*)eEyeEntryData.m_address))
    {
      eEyeEntryData.m_address[0] = '\0';
    }
    // 读取名称
    m_viewWrapper.GetPickName((char*)eEyeEntryData.m_name);
    if(::strlen((char*)eEyeEntryData.m_name) > 0)
    {
      eEyeEntryData.m_name[::strlen((char*)eEyeEntryData.m_name) - 1] = '\0';
    }
    else
    {
      ::strcpy((char*)eEyeEntryData.m_name, (char*)eEyeEntryData.m_address);
    }

    eEyeEntryData.m_x = pickPos.m_x;
    eEyeEntryData.m_y = pickPos.m_y;
    eEyeEntryData.m_type = TVT_NormalCamera;


    //读取网格ID和路网ID
    long parcelIdx = m_net->GetParcelID(PT_Real, pickPos);
    if (parcelIdx >= 0)
    {
      INetParcel *netParcel = m_net->GetParcel(PT_Real, parcelIdx);
      if (netParcel)
      {
        NetPosition netPosition;
        netPosition.m_parcelIdx = parcelIdx;
        netPosition.m_realPosition.m_x = pickPos.m_x;
        netPosition.m_realPosition.m_y = pickPos.m_y;
        INetLink *netLink = netParcel->GetNearest(netPosition, 1000);
        if (netLink)
        {
          eEyeEntryData.m_linkId = netPosition.m_linkIdx;
          addStatus = m_userDataWrapper.AddUserEEyeData(parcelIdx, eEyeEntryData);          
        }
      }
    }
  }

  CMessageDialogEvent dialogEvent(this, DHT_MapHook, NULL);
  if (addStatus)
  {
    CMessageDialogHook::ShowMessageDialog(MB_NONE, "保存成功！", dialogEvent);    
  }
  else
  {    
    CMessageDialogHook::ShowMessageDialog(MB_NONE, "保存失败！", dialogEvent);
  }
  Sleep(500);
  CMessageDialogHook::CloseMessageDialog();
}

void UeGui::CMapHook::RefreshSrcModalBtnStatus()
{
  m_screenMoadlBtn.SetEnable(true);
  short planState = m_routeWrapper.GetPlanState();
  if ((UeRoute::PS_DemoGuidance == planState) || (UeRoute::PS_RealGuidance == planState))
  {
    UeMap::ViewOpeMode viewMode = VM_Unknown;
    CViewState* curViewState = m_viewWrapper.GetMainViewState();
    if (curViewState)
    {
      viewMode = m_viewWrapper.GetViewOpeMode((UeMap::ViewType)curViewState->GetType());
    }

    if (UeMap::VM_Browse == viewMode)                                    
    {
      m_screenMoadlBtn.SetEnable(false);
    }
    else if (m_viewWrapper.IsGuidanceViewShown())
    {
      m_screenMoadlBtn.SetEnable(false);
    }
  }
  else
  {
    m_screenMoadlBtn.SetEnable(false);
  }
}

void UeGui::CMapHook::RefreshSysTime()
{
  //每分钟刷新一次
  short hour(0), minute(0);
  if(m_gps->IsLive())
  {
    GpsBasic pos;
    m_gps->GetPos(pos, false);
    hour = pos.m_localTime.m_hour;
    minute = pos.m_localTime.m_min;
  }
  else
  {
    CTimeBasic::TimeReport report;
    CTimeBasic timer;
    timer.GetNow(report);
    hour = report.m_hour;
    minute = report.m_minute;
  }
  short curTime = hour * 100 + minute;
  if (m_sysTime != curTime)
  {    
    m_sysTime = curTime;
    char buf[8] = {};
    ::sprintf(buf, "%02d:%02d", hour, minute);
    m_timerBtn.SetCaption(buf);
    //如果是导航过程中则不主动刷新，而是靠导航时的地图刷新来刷新时间
    unsigned short planState = m_routeWrapper.GetPlanState();
    if ((UeRoute::PS_RealGuidance != planState) && (UeRoute::PS_DemoGuidance != planState) && (!CAGGView::IsScrolling()))
    {
      Refresh();
    }
  }
}