#include "mapsimulationmenuhook.h"
#include "maphook.h"

using namespace UeGui;

CMapSimulationMenuHook::CMapSimulationMenuHook() : m_parentHook(NULL), m_routeWrapper(CRouteWrapper::Get()),
m_viewWrapper(CViewWrapper::Get()), m_speedIndex(0), m_simulationStatus(false), m_expandStatus(false)
{
  //地图界面渲染完成后不需要释放图片资源，提高效率
  m_needReleasePic = false;
  //注意：模拟导航速度为当前传入的速度值加上上次传入的速度作为当前模拟速度
  SpeedInfo speedInfo;
  speedInfo.m_speed = INITIAL_DEMOSPEED;
  ::strcpy(speedInfo.m_speedText, "30 km/h");
  m_speedList.push_back(speedInfo);
  speedInfo.m_speed = INITIAL_DEMOSPEED;
  ::strcpy(speedInfo.m_speedText, "60 km/h");
  m_speedList.push_back(speedInfo);
  speedInfo.m_speed = INITIAL_DEMOSPEED;
  ::strcpy(speedInfo.m_speedText, "90 km/h");
  m_speedList.push_back(speedInfo);
  speedInfo.m_speed = INITIAL_DEMOSPEED;
  ::strcpy(speedInfo.m_speedText, "120 km/h");
  m_speedList.push_back(speedInfo);
  speedInfo.m_speed = INITIAL_DEMOSPEED;
  ::strcpy(speedInfo.m_speedText, "150 km/h");
  m_speedList.push_back(speedInfo);
  speedInfo.m_speed = INITIAL_DEMOSPEED;
  ::strcpy(speedInfo.m_speedText, "180 km/h");
  m_speedList.push_back(speedInfo);
}

CMapSimulationMenuHook::~CMapSimulationMenuHook()
{
  m_elements.clear();
  m_ctrlNames.clear();
  m_imageNames.clear();
}

bool CMapSimulationMenuHook::operator ()()
{
  return false;
}

void UeGui::CMapSimulationMenuHook::Show( bool show /*= true */ )
{
  CAggHook::Show(show);
  if (show)
  {
    ExpandMenu(false);
  }  
}

tstring CMapSimulationMenuHook::GetBinaryFileName()
{
  return _T("mapsimulationmenuhook.bin");
}

void CMapSimulationMenuHook::MakeNames()
{
  m_ctrlNames.erase(m_ctrlNames.begin(), m_ctrlNames.end());
  m_ctrlNames.insert(GuiName::value_type(MapSimulationMenuHook_ExpandBtn,	"ExpandBtn"));
  m_ctrlNames.insert(GuiName::value_type(MapSimulationMenuHook_CloseBtn,	"CloseBtn"));
  m_ctrlNames.insert(GuiName::value_type(MapSimulationMenuHook_SlowlyDownBtn,	"SlowlyDownBtn"));
  m_ctrlNames.insert(GuiName::value_type(MapSimulationMenuHook_SlowlyDownLabel,	"SlowlyDownLabel"));
  m_ctrlNames.insert(GuiName::value_type(MapSimulationMenuHook_Delimiter1,	"Delimiter1"));
  m_ctrlNames.insert(GuiName::value_type(MapSimulationMenuHook_SpeedLevelBtn,	"SpeedLevelBtn"));
  m_ctrlNames.insert(GuiName::value_type(MapSimulationMenuHook_SpeedLevelLabel,	"SpeedLevelLabel"));
  m_ctrlNames.insert(GuiName::value_type(MapSimulationMenuHook_Delimiter2,	"Delimiter2"));
  m_ctrlNames.insert(GuiName::value_type(MapSimulationMenuHook_SpeedUpBtn,	"SpeedUpBtn"));
  m_ctrlNames.insert(GuiName::value_type(MapSimulationMenuHook_SpeedUpLabel,	"SpeedUpLabel"));
  m_ctrlNames.insert(GuiName::value_type(MapSimulationMenuHook_Delimiter3,	"Delimiter3"));
  m_ctrlNames.insert(GuiName::value_type(MapSimulationMenuHook_PauseBtn,	"PauseBtn"));
  m_ctrlNames.insert(GuiName::value_type(MapSimulationMenuHook_PauseLabel,	"PauseLabel"));
  m_ctrlNames.insert(GuiName::value_type(MapSimulationMenuHook_PauseBtnR,	"PauseBtnR"));
  m_ctrlNames.insert(GuiName::value_type(MapSimulationMenuHook_StopSimulationBtn,	"StopSimulationBtn"));
  m_ctrlNames.insert(GuiName::value_type(MapSimulationMenuHook_StopSimulationLabel,	"StopSimulationLabel"));
}

void CMapSimulationMenuHook::MakeControls()
{
  //展开菜单按钮
  m_expandBtn.SetCenterElement(GetGuiElement(MapSimulationMenuHook_ExpandBtn));
  //关闭菜单按钮
  m_closeBtn.SetCenterElement(GetGuiElement(MapSimulationMenuHook_CloseBtn));
  //减速
  m_slowlyDownBtn.SetCenterElement(GetGuiElement(MapSimulationMenuHook_SlowlyDownBtn));
  m_slowlyDownBtn.SetLabelElement(GetGuiElement(MapSimulationMenuHook_SlowlyDownLabel));
  //当前速度
  m_speedLevelBtn.SetCenterElement(GetGuiElement(MapSimulationMenuHook_SpeedLevelBtn));
  m_speedLevelBtn.SetLabelElement(GetGuiElement(MapSimulationMenuHook_SpeedLevelLabel));
  //加速
  m_speedUpBtn.SetCenterElement(GetGuiElement(MapSimulationMenuHook_SpeedUpBtn));
  m_speedUpBtn.SetLabelElement(GetGuiElement(MapSimulationMenuHook_SpeedUpLabel));
  //暂停
  m_pauseBtn.SetCenterElement(GetGuiElement(MapSimulationMenuHook_PauseBtn));
  m_pauseBtn.SetRightElement(GetGuiElement(MapSimulationMenuHook_PauseBtnR));
  m_pauseBtn.SetLabelElement(GetGuiElement(MapSimulationMenuHook_PauseLabel));
  //停止模拟
  m_stopSimulation.SetCenterElement(GetGuiElement(MapSimulationMenuHook_StopSimulationBtn));
  m_stopSimulation.SetLabelElement(GetGuiElement(MapSimulationMenuHook_StopSimulationLabel));
  //分割符
  m_delimiter1.SetCenterElement(GetGuiElement(MapSimulationMenuHook_Delimiter1));;
  m_delimiter2.SetCenterElement(GetGuiElement(MapSimulationMenuHook_Delimiter2));
  m_delimiter3.SetCenterElement(GetGuiElement(MapSimulationMenuHook_Delimiter3));
}

short CMapSimulationMenuHook::MouseDown(CGeoPoint<short> &scrPoint)
{
  //是否需要刷新
  bool needRefresh = false;
  short ctrlType = CAggHook::MouseDown(scrPoint);
  switch(ctrlType)
  {
  case MapSimulationMenuHook_ExpandBtn:
    {
      m_expandBtn.MouseDown();
      needRefresh = true;
    }
    break;
  case MapSimulationMenuHook_CloseBtn:
    {
      m_closeBtn.MouseDown();
      needRefresh = true;
    }
    break;
  case MapSimulationMenuHook_SlowlyDownBtn:
  case MapSimulationMenuHook_SlowlyDownLabel:
    {
      m_slowlyDownBtn.MouseDown();
      needRefresh = true;
    }
    break;
  case MapSimulationMenuHook_SpeedUpBtn:
  case MapSimulationMenuHook_SpeedUpLabel:
    {
      m_speedUpBtn.MouseDown();
      needRefresh = true;
    }
    break;
  case MapSimulationMenuHook_PauseBtn:
  case MapSimulationMenuHook_PauseBtnR:
  case MapSimulationMenuHook_PauseLabel:
    {
      m_pauseBtn.MouseDown();
      needRefresh = true;
    }
    break;
  case MapSimulationMenuHook_StopSimulationBtn:
  case MapSimulationMenuHook_StopSimulationLabel:
    {
      m_stopSimulation.MouseDown();
      needRefresh = true;
    }
    break;
  default:
    assert(false);
    break;
  }

  if (needRefresh)
  {
    this->Refresh();
  }
  return ctrlType;
}

short CMapSimulationMenuHook::MouseMove(CGeoPoint<short> &scrPoint)
{
  return -1;
}

short CMapSimulationMenuHook::MouseUp(CGeoPoint<short> &scrPoint)
{
  //是否需要刷新
  bool needRefresh = false;
  short ctrlType = CAggHook::MouseUp(scrPoint);
  switch(ctrlType)
  {
  case MapSimulationMenuHook_ExpandBtn:
    {
      m_expandBtn.MouseUp();
      needRefresh = true;
      ExpandMenu(true);
    }
    break;
  case MapSimulationMenuHook_CloseBtn:
    {
      m_closeBtn.MouseUp();
      needRefresh = true;
      ExpandMenu(false);
    }
    break;
  case MapSimulationMenuHook_SlowlyDownBtn:
  case MapSimulationMenuHook_SlowlyDownLabel:
    {
      m_slowlyDownBtn.MouseUp();
      needRefresh = true;
      if (m_slowlyDownBtn.IsEnable())
      {
        //减速
        SlowlyDown();
      }
    }
    break;
  case MapSimulationMenuHook_SpeedUpBtn:
  case MapSimulationMenuHook_SpeedUpLabel:
    {
      m_speedUpBtn.MouseUp();
      needRefresh = true;
      if (m_speedUpBtn.IsEnable())
      {
        //加速
        SpeedUp();
      }
    }
    break;
  case MapSimulationMenuHook_PauseBtn:
  case MapSimulationMenuHook_PauseBtnR:
  case MapSimulationMenuHook_PauseLabel:
    {
      m_pauseBtn.MouseUp();
      needRefresh = true;
      if (m_simulationStatus)
      {
        //暂停模拟导航
        if (UeRoute::PEC_Success == m_routeWrapper.PauseDemo())
        {
          m_simulationStatus = false;
          ResetSpeedBtnCaption();
        }
      }
      else
      {
        //继续模拟导航
        if (UeRoute::PEC_Success == m_routeWrapper.ResumeDemo(0))
        {
          m_simulationStatus = true;
          ResetSpeedBtnCaption();
        }        
      }
    }
    break;
  case MapSimulationMenuHook_StopSimulationBtn:
  case MapSimulationMenuHook_StopSimulationLabel:
    {
      m_stopSimulation.MouseUp();
      needRefresh = false;
      //停止模拟导航
      if (m_parentHook)
      {
        CMapHook* mapHook = dynamic_cast<CMapHook*>(m_parentHook);
        mapHook->StopDemo();
        m_simulationStatus = false;
      }
    }
    break;
  default:
    assert(false);
    break;
  }

  if (needRefresh)
  {
    this->Refresh();
  }
  return ctrlType;
}

void UeGui::CMapSimulationMenuHook::SetParentHook( CAggHook* parentHook )
{
  m_parentHook = parentHook;
}

void UeGui::CMapSimulationMenuHook::ExpandMenu( bool bExpand /*= true*/ )
{
  CMapHook* mapHook = NULL;
  if (m_parentHook)
  {
    mapHook = dynamic_cast<CMapHook*>(m_parentHook);
  }
  if (NULL == mapHook)
  {
    return;
  }
  //保存当前菜单展开状态
  m_expandStatus = bExpand;
  //读取当前规划状态
  short planState = m_routeWrapper.GetPlanState();

  if (bExpand)
  {
    //显示菜单栏
    ShowMenuBar(true, bExpand);
    //开启界面切换定时器
    mapHook->RestarGuiTimer();
    mapHook->ShowMinimizeBtn();      
    mapHook->ShowMapAzimuthBtn();
    mapHook->ShowMapScalingBtn();
    mapHook->ShowDetailBtn1();
    mapHook->ShowDetailBtn2(false);
    mapHook->ShowGuideInfoBtn(false);
    mapHook->ShowAddElecEyeBtn(false); 
    mapHook->ShowSetDestPointBtn(false);
    mapHook->ShowFixedPostionBtn(false);
    mapHook->ShowCompass(false);
    mapHook->ShowElecEye(false);
    mapHook->RefreshSrcModalBtnStatus();
    //停止导航按钮
    m_stopSimulation.SetVisible(true);
  }
  else
  {
    //隐藏菜单栏
    ShowMenuBar(true, bExpand);
    //开启界面切换定时器
    mapHook->CloseGuiTimer();
    mapHook->ShowMinimizeBtn(false);
    mapHook->ShowAddElecEyeBtn();
    mapHook->ShowDetailBtn1(false);
    mapHook->ShowDetailBtn2(false);
    mapHook->ShowMapAzimuthBtn(false);
    mapHook->ShowMapScalingBtn(false);
    mapHook->ShowSetDestPointBtn(false);
    mapHook->ShowFixedPostionBtn(false);
    //停止导航按钮
    m_stopSimulation.SetVisible(false);
  }
  //动态更新界面
  DynamicUpdate(bExpand, planState, VM_Guidance);
}

void UeGui::CMapSimulationMenuHook::Update( short type )
{
  //读取当前规划状态
  short planState = m_routeWrapper.GetPlanState();
  //动态更新界面
  DynamicUpdate(m_expandStatus, planState, VM_Guidance);
}

void UeGui::CMapSimulationMenuHook::DynamicUpdate( bool bExpand, short planState, UeMap::ViewOpeMode viewMode )
{
  CMapHook* mapHook = NULL;
  if (m_parentHook)
  {
    mapHook = dynamic_cast<CMapHook*>(m_parentHook);
  }
  if (NULL == mapHook)
  {
    return;
  }

  if (bExpand)
  {
    //更新切换屏幕模式按钮状态
    mapHook->RefreshSrcModalBtnStatus();
  }
  else
  {
    //检查是否有电子眼提示
    if (mapHook->HaveElecEyePrompt())
    {
      mapHook->ShowElecEye();
      mapHook->ShowCompass(false);
    }
    else
    {
      mapHook->ShowElecEye(false);
      mapHook->ShowCompass();
    }      
    //如果显示路口放大图
    if (m_viewWrapper.IsGuidanceViewShown())
    {
      //隐藏下一道路
      mapHook->ShowGuideInfoBtn(false);
    }
    else
    {
      //显示下一道路
      mapHook->ShowGuideInfoBtn();
    }
  }

  //判断是否要显示时间按钮
  if (m_viewWrapper.IsGuidanceViewShown())
  {
    mapHook->ShowTimeBtn(false);
  }
  else
  {
    if (m_viewWrapper.IsNeedRenderGuidanceView())
    {
      mapHook->ShowTimeBtn(false);
    }
    else
    {
      mapHook->ShowTimeBtn();
    }
  }
}

void UeGui::CMapSimulationMenuHook::ShowMenuBar( bool bShow, bool bExpand )
{
  if (bShow)
  {
    if (bExpand)
    {
      //展开菜单
      m_expandBtn.SetVisible(false);
      m_closeBtn.SetVisible(true);
      m_slowlyDownBtn.SetVisible(true);
      m_speedLevelBtn.SetVisible(true);
      m_speedUpBtn.SetVisible(true);
      m_pauseBtn.SetVisible(true);    
      m_delimiter1.SetVisible(true);
      m_delimiter2.SetVisible(true);
      m_delimiter3.SetVisible(true);
    }
    else
    {
      //收缩菜单
      m_expandBtn.SetVisible(true);
      m_closeBtn.SetVisible(false);
      m_slowlyDownBtn.SetVisible(false);
      m_speedLevelBtn.SetVisible(false);
      m_speedUpBtn.SetVisible(false);
      m_pauseBtn.SetVisible(false);    
      m_delimiter1.SetVisible(false);
      m_delimiter2.SetVisible(false);
      m_delimiter3.SetVisible(false);
    }
  }
  else
  {
    //隐藏菜单
    m_expandBtn.SetVisible(false);
    m_closeBtn.SetVisible(false);
    m_slowlyDownBtn.SetVisible(false);
    m_speedLevelBtn.SetVisible(false);
    m_speedUpBtn.SetVisible(false);
    m_pauseBtn.SetVisible(false);    
    m_delimiter1.SetVisible(false);
    m_delimiter2.SetVisible(false);
    m_delimiter3.SetVisible(false);
  }
}

void UeGui::CMapSimulationMenuHook::ReseSimulation()
{
  //当前速度索引
  m_speedIndex = 0;
  m_simulationStatus = true;
  RefreshSpeedLabel();
  ResetSpeedBtnStatus();
  ResetSpeedBtnCaption();
}

void UeGui::CMapSimulationMenuHook::SlowlyDown()
{
  if (m_speedIndex >= 1)
  {
    m_speedIndex--;
    m_routeWrapper.ResumeDemo(-1 * m_speedList[m_speedIndex].m_speed);
    if (!m_simulationStatus)
    {
      m_routeWrapper.PauseDemo();
    }
    RefreshSpeedLabel();
    ResetSpeedBtnStatus();
  }
}

void UeGui::CMapSimulationMenuHook::SpeedUp()
{
  if (m_speedIndex < m_speedList.size() - 1)
  {
    m_speedIndex++;
    m_routeWrapper.ResumeDemo(m_speedList[m_speedIndex].m_speed);
    if (!m_simulationStatus)
    {
      m_routeWrapper.PauseDemo();
    }
    RefreshSpeedLabel();
    ResetSpeedBtnStatus();
  }
}

void UeGui::CMapSimulationMenuHook::ResetSpeedBtnStatus()
{
  if (m_speedIndex <= 0)
  {
    m_slowlyDownBtn.SetEnable(false);
  }
  else if (m_speedIndex  > 0)
  {
    m_slowlyDownBtn.SetEnable(true);
  }
 

  if (m_speedIndex >= m_speedList.size() - 1)
  {
    m_speedUpBtn.SetEnable(false);
  }
  else if (m_speedIndex < m_speedList.size() - 1)
  {
    m_speedUpBtn.SetEnable(true);
  }
}

void UeGui::CMapSimulationMenuHook::ResetSpeedBtnCaption()
{
  if (m_simulationStatus)
  {
    m_pauseBtn.SetCaption("暂停");
  }
  else
  {
    m_pauseBtn.SetCaption("继续");
  }
}

void UeGui::CMapSimulationMenuHook::RefreshSpeedLabel()
{
  if ((m_speedIndex >= 0) && (m_speedIndex <= m_speedList.size() - 1))
  {
    m_speedLevelBtn.SetCaption(m_speedList[m_speedIndex].m_speedText);
  }  
}