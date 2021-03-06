// WeighToolView.cpp : implementation of the CWeighToolView class
//

#include "stdafx.h"
#include "WeighTool.h"
#include "Splash.h"
#include "WeighToolDoc.h"
#include "WeighToolView.h"
#include "Password.h"
#include <math.h>
#include "WeighSetting.h"

#define FAIL                102
#define PASS                103

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CWeighToolView

IMPLEMENT_DYNCREATE(CWeighToolView, CFormView)

BEGIN_MESSAGE_MAP(CWeighToolView, CFormView)
	//{{AFX_MSG_MAP(CWeighToolView)
	ON_CBN_SELCHANGE(IDC_COMBO_COMPORT, OnSelchangeComboComport)
	ON_BN_CLICKED(IDC_BTN_START, OnBtnStart)
	ON_WM_TIMER()
	ON_EN_CHANGE(IDC_EDIT_CARTONNUM, OnChangeEditCartonNum)
	ON_BN_CLICKED(IDC_BTN_SETTING, OnBtnSetting)
	ON_BN_CLICKED(IDC_BTN_REFRESH, OnBtnRefresh)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWeighToolView construction/destruction

void StartDoWight(LPVOID x)
{
	CWeighToolView *Sv = (CWeighToolView *)x;
	Sv->DoWeigh();
}


CWeighToolView::CWeighToolView()
	: CFormView(CWeighToolView::IDD)
{
	//{{AFX_DATA_INIT(CWeighToolView)
	m_iComport = -1;
	m_dHighWeigh = 0.0;
	m_dLowWeigh = 0.0;
	m_csCartonNum = _T("");
	m_csCurrentColor = _T("");
	m_csCurrentOrder = _T("");
	m_csCurrentProduct = _T("");
	m_csCurrentLine = _T("");
	//}}AFX_DATA_INIT
	// TODO: add construction code here
}


CWeighToolView::~CWeighToolView()
{
}


void CWeighToolView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWeighToolView)
	DDX_Control(pDX, IDC_STATIC_CURRENT_LINE, m_ctrCurrentLine);
	DDX_Control(pDX, IDC_EDIT_CARTONNUM, m_ctrCartonNum);
	DDX_Control(pDX, IDC_STATIC_CURRENT_COUNT, m_cCurrentCount);
	DDX_Control(pDX, IDC_STATIC_CURRENT_PRODUCT, m_ctrCurrentProduct);
	DDX_Control(pDX, IDC_STATIC_CURRENT_ORDER, m_ctrCurrentOrder);
	DDX_Control(pDX, IDC_STATIC_CURRENT_COLOR, m_ctrCurrentColor);
	DDX_Control(pDX, IDC_STATIC_RESULT, m_cResult);
	DDX_Control(pDX, IDC_EDIT_LOW_WEGIH, m_ctrLowWeigh);
	DDX_Control(pDX, IDC_EDIT_HIGH_WEGIH, m_ctrHighWeigh);
	DDX_Control(pDX, IDC_LIST_STATE, m_state_list);
	DDX_Control(pDX, IDC_STATIC_CURR, m_ctrWeigh);
	DDX_Control(pDX, IDC_STATIC_UNIT, m_ctrUnit);
	DDX_Control(pDX, IDC_COMBO_COMPORT, m_ctrComport);
	DDX_CBIndex(pDX, IDC_COMBO_COMPORT, m_iComport);
	DDX_Text(pDX, IDC_EDIT_HIGH_WEGIH, m_dHighWeigh);
	DDX_Text(pDX, IDC_EDIT_LOW_WEGIH, m_dLowWeigh);
	DDX_Text(pDX, IDC_EDIT_CARTONNUM, m_csCartonNum);
	DDX_Text(pDX, IDC_STATIC_CURRENT_COLOR, m_csCurrentColor);
	DDX_Text(pDX, IDC_STATIC_CURRENT_ORDER, m_csCurrentOrder);
	DDX_Text(pDX, IDC_STATIC_CURRENT_PRODUCT, m_csCurrentProduct);
	DDX_Text(pDX, IDC_STATIC_CURRENT_LINE, m_csCurrentLine);
	//}}AFX_DATA_MAP
}


BOOL CWeighToolView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CFormView::PreCreateWindow(cs);
}


void CWeighToolView::OnInitialUpdate()
{
	CFormView::OnInitialUpdate();
	GetParentFrame()->RecalcLayout();
	ResizeParentToFit();

	//装载端口
	this->LoadComPort();

	//设置字体
	this->SetMyFont();

	//初始化ini文件
	this->m_Config.InitConfigPath();

	//读取配置文件
	this->m_Config.ReadConfig();

	m_csCurrentOrder = ((CWeighToolApp *)AfxGetApp())->InputOrder;
	m_csCurrentLine  = ((CWeighToolApp *)AfxGetApp())->InputProductionLine;

	//灰掉控件
	this->SetUIDisableAll();

	//获取项目列表
	if(!this->GetProductOrderInfo())
	{
		InsertListInfo("Access to project orders information failure...",2);
		return;
	}
	else
	{
		UpdateData(FALSE);
		InsertListInfo("To obtain project orders information success...",0);
	}
	
	//获取重量范围
	if(!this->GetSetting())
	{
		InsertListInfo("Failure to obtain weight range...",2);
		return;
	}
	else
	{
		UpdateData(FALSE);
		InsertListInfo("Success in weight range...",0);
	}

	if(!this->Load_Current_Product_Count())
	{
		InsertListInfo("To obtain the number of current orders has failed",2);
		return;
	}
	InsertListInfo("Obtain the number of current orders has been successful",0);
	this->UpdateCount(this->m_csCurrentCount);
}


/////////////////////////////////////////////////////////////////////////////
// CWeighToolView diagnostics

#ifdef _DEBUG
void CWeighToolView::AssertValid() const
{
	CFormView::AssertValid();
}


void CWeighToolView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}


CWeighToolDoc* CWeighToolView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CWeighToolDoc)));
	return (CWeighToolDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CWeighToolView message handlers

void CWeighToolView::LoadComPort()
{
	CString str;
	this->m_ctrComport.ResetContent();
	int portnum = 0;   
	CHAR port[MAX_PATH];   
	COMMCONFIG comcfg;   
	ULONG BuffSize = sizeof(port);
	for(int i = 0; i< 255 ; i++) 
	{
		m_comnum[i] = 0;
	}
	for( i = 1; i< 255 ; i++)   
	{   		
		_stprintf(port,_T("COM%d"),i);   
		BuffSize = sizeof(port);   
		if(GetDefaultCommConfig(port,&comcfg,&BuffSize))   
		{
			this->m_ctrComport.AddString(port); 
			m_comnum[portnum++] = i;
		}			
		memset(port,0,sizeof(port));   
	} 	
	m_ctrComport.SetCurSel(-1);	
}


void CWeighToolView::OnSelchangeComboComport() 
{
	m_iComport= m_ctrComport.GetCurSel();	
	m_comport = m_iComport;
	m_comport = m_comnum[m_comport];

	if(InitComPort())
	{
		InsertListInfo("Init Comport Pass!",0);
		this->SetTimer(1,500,0);

		this->m_ctrCartonNum.EnableWindow(TRUE);
		this->m_ctrCartonNum.SetFocus();
	}
	else
	{
		this->m_ctrCartonNum.EnableWindow(FALSE);
		InsertListInfo("Init Comport Fail!",2);
	}
}


BOOL CWeighToolView::InitComPort()
{
	try
	{
		//初始化串口
		this->m_Serial.m_nBaudRate = 9600;
		this->m_Serial.m_nPort=this->m_comport;
		
		if(this->m_Serial.Port_Init())
		{	
			this->m_RSData.m_hPort = this->m_Serial.CommHandle;  //将之前初始化好的CMySerial Port 句柄传递给CRSData句柄以便读写数据
			
		}
		else
		{
			return false;
		}
	}
	catch (int k)
	{
		return false;
	}
	
	return true;
}


void CWeighToolView::DoWeigh() 
{
	int RS232Error,SN_Long;
	char  read_buffer[200];
	DWORD dwdTotalBytesSent=0;
	DWORD dwdBytesReceived=0;
	char *PointorSN,*PointorEnd;
	char csWeigh[10];
	memset(read_buffer, 0, sizeof(read_buffer));
	
	strcpy (read_buffer,"" );
	RS232Error =ReadFile (this->m_RSData.m_hPort,read_buffer, 50, &dwdBytesReceived, NULL);
	PurgeComm (this->m_RSData.m_hPort, PURGE_TXCLEAR | PURGE_RXCLEAR);//FlushInQ (comport);    
	
	PointorSN=strstr(read_buffer,",NT,"); //,NT,+    0.0g
	if(PointorSN!=0)
	{
		PointorSN=PointorSN+4;
		PointorEnd=PointorSN+8;
		SN_Long=PointorEnd-PointorSN;
		for(int b=0;b<SN_Long;b++)
		{
			csWeigh[b]=*(PointorSN+b);
			if(b+1==SN_Long)
				csWeigh[b+1]='\0';
		}

		CString temp;
		temp.Format("%s",csWeigh);
		temp.Replace(" ","");

		double tempdWeigh;
		sscanf(temp, "%lf", &tempdWeigh);
		temp.Format("%.1f",tempdWeigh);

		this->m_dWeigh=tempdWeigh;
		if(m_dWeigh>this->m_dHighWeigh || m_dWeigh<this->m_dLowWeigh)
		{
			this->m_ctrWeigh.SetTextColor(RGB(255,0,0));
		}
		else
		{
			this->m_ctrWeigh.SetTextColor(RGB(0,255,0));
		}

		this->m_ctrWeigh.SetWindowText(temp);
	}
	else
	{
		this->m_ctrWeigh.SetWindowText("");
		InsertListInfo("Electronic communication and PC has not been successful communication!",2);
	}
}


void CWeighToolView::OnBtnStart() 
{
	m_hThread =
		CreateThread (NULL,  
		0,     
		(LPTHREAD_START_ROUTINE)StartDoWight,
		(void *)this,  
		0,     
		&m_dThreadId);
}


void CWeighToolView::OnTimer(UINT nIDEvent) 
{
	if(nIDEvent == 1)
	{
		OnBtnStart();
	}
}


void CWeighToolView::SetMyFont()
{
	//===============set imagelist=====================================
	HICON hIcon[3];
	hIcon[0]=AfxGetApp()->LoadIcon(IDI_PASS);
	hIcon[1]=AfxGetApp()->LoadIcon(IDI_ERRORS);
	hIcon[2]=AfxGetApp()->LoadIcon(IDI_STOP);
	if(m_imageList.m_hImageList!=NULL)
		m_imageList.DeleteImageList();
	m_imageList.Create(16,16,ILC_COLOR32,3,3);
	m_imageList.SetBkColor(RGB(255,255,255));
	m_imageList.Add(hIcon[0]);
	m_imageList.Add(hIcon[1]);
	m_imageList.Add(hIcon[2]);	
	
	//==========init state list=====================================
    DWORD dwStyle;
	LVCOLUMN   Column;	
	
	m_state_list.SetImageList(&m_imageList,LVSIL_SMALL);
	dwStyle = m_state_list.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;//选中某行使整行高亮（只适用与report风格的listctrl）
	//dwStyle |= LVS_EX_GRIDLINES;//网格线（只适用与report风格的listctrl）
	//dwStyle |= LVS_EX_CHECKBOXES;//item前生成checkbox控件
	m_state_list.SetExtendedStyle(dwStyle); //设置扩展风格

	//((CWnd*)GetDlgItem(IDC_LIST_STATE))->SetFont(&m_sysfont);
	Column.mask =   LVCF_TEXT|LVCF_FMT|LVCF_WIDTH;
	Column.cchTextMax =   30;   
	Column.cx =   170;     //column宽度，不工作！   
	Column.fmt =   LVCFMT_LEFT;     //不工作   
	Column.iImage =   0;     
	Column.iOrder =   0;     //不明白干什么用   
	Column.iSubItem =   8;        
	Column.pszText =   "Time";   
	m_state_list.InsertColumn(0,&Column);
	Column.pszText =   "Status"; 
	Column.cx =   400;
	m_state_list.InsertColumn(1,&Column);
	
	this->m_ctrWeigh.SetFontSize(100);
	this->m_ctrWeigh.SetFontName("Arial");
	this->m_ctrUnit.SetFontSize(100);
	this->m_ctrUnit.SetFontName("Arial");

	this->m_cResult.SetFontSize(90);
	this->m_cResult.SetFontName("Arial");

	this->m_cCurrentCount.SetFontSize(30);
	this->m_cCurrentCount.SetFontName("Arial");
	this->m_cCurrentCount.SetTextColor(RGB(0,0,255));

	this->m_ctrCurrentProduct.SetFontSize(18);
	this->m_ctrCurrentProduct.SetFontName("Arial");
	this->m_ctrCurrentProduct.SetTextColor(RGB(0,0,255));
	
	this->m_ctrCurrentOrder.SetFontSize(18);
	this->m_ctrCurrentOrder.SetFontName("Bold");
	this->m_ctrCurrentOrder.SetTextColor(RGB(0,0,255));
	
	this->m_ctrCurrentColor.SetFontSize(18);
	this->m_ctrCurrentColor.SetFontName("Bold");
	this->m_ctrCurrentColor.SetTextColor(RGB(0,0,255));

	this->m_ctrCurrentLine.SetFontSize(18);
	this->m_ctrCurrentLine.SetFontName("Bold");
	this->m_ctrCurrentLine.SetTextColor(RGB(0,0,255));

	VERIFY(m_staticfont.CreateFont(
		18,                        // nHeight
		0,                         // nWidth
		0,                         // nEscapement
		0,                         // nOrientation
		FW_NORMAL,                 // nWeight
		FALSE,                     // bItalic
		FALSE,                     // bUnderline
		0,                         // cStrikeOut
		ANSI_CHARSET,              // nCharSet
		OUT_DEFAULT_PRECIS,        // nOutPrecision
		CLIP_DEFAULT_PRECIS,       // nClipPrecision
		DEFAULT_QUALITY,           // nQuality
		DEFAULT_PITCH | FF_SWISS,  // nPitchAndFamily
		"Arial"));                 // lpszFacename

	VERIFY(m_staticfont1.CreateFont(
		32,                        // nHeight
		0,                         // nWidth
		0,                         // nEscapement
		0,                         // nOrientation
		FW_NORMAL,                 // nWeight
		FALSE,                     // bItalic
		FALSE,                     // bUnderline
		0,                         // cStrikeOut
		ANSI_CHARSET,              // nCharSet
		OUT_DEFAULT_PRECIS,        // nOutPrecision
		CLIP_DEFAULT_PRECIS,       // nClipPrecision
		DEFAULT_QUALITY,           // nQuality
		DEFAULT_PITCH | FF_SWISS,  // nPitchAndFamily
		"Arial"));                 // lpszFacename
	
	((CWnd*)GetDlgItem(IDC_STATIC_CARTONNUM))->SetFont(&m_staticfont);
	((CWnd*)GetDlgItem(IDC_EDIT_CARTONNUM))->SetFont(&m_staticfont1);
	((CWnd*)GetDlgItem(IDC_EDIT_HIGH_WEGIH))->SetFont(&m_staticfont);
	((CWnd*)GetDlgItem(IDC_EDIT_LOW_WEGIH))->SetFont(&m_staticfont);	
}


void CWeighToolView::InsertListInfo(CString state,int imageIndex)
{
	CString date;
	date=GetCurTimes();
	m_state_list.InsertItem(m_state_list.GetItemCount(),date,imageIndex);
	m_state_list.SetItemText(m_state_list.GetItemCount()-1,1,state);
	m_state_list.EnsureVisible(m_state_list.GetItemCount()-1,TRUE);
}


CString CWeighToolView::GetCurTimes()
{
	CTime t=CTime::GetCurrentTime();
	CString str=t.Format( "%Y-%m-%d  %H:%M:%S");
	return str;
}


void CWeighToolView::SetUIDisableAll()
{
	this->m_ctrCartonNum.EnableWindow(FALSE);
}


CString CWeighToolView::VariantToCString(VARIANT   var)   
{   
	CString strValue;   
	_variant_t var_t;   
	_bstr_t bst_t;   
	//time_t cur_time;   
	CTime time_value;   
	COleCurrency var_currency; 
	COleDateTime cur_time;
    
	switch(var.vt)   
	{   
	case   VT_EMPTY:   
		strValue   =   _T("");   
		break;   
	case   VT_UI1:   
		strValue.Format("%d",var.bVal);   
		break;   
	case   VT_I2:   
		strValue.Format("%d",var.iVal);   
		break;   
	case   VT_I4:   
		strValue.Format("%d",var.lVal);   
		break;   
	case   VT_R4:   
		strValue.Format("%f",var.fltVal);   
		break;   
	case   VT_R8:   
		strValue.Format("%f",var.dblVal);   
		break;   
	case   VT_CY:   
		var_currency   =   var;   
		strValue   =   var_currency.Format(0);   
		break;   
	case   VT_BSTR:   
		var_t   =   var;   
		bst_t   =   var_t;   
		strValue.Format   ("%s",(const   char*)bst_t);   
		break;   
	case   VT_NULL:   
		strValue   =   _T("");   
		break;   
	case   VT_DATE:   
		//cur_time   =   (long)var.date;   
		// time_value   =   cur_time;   
		// strValue   =   time_value.Format("%Y-%m-%d");  
		cur_time   =   var; 
		strValue   =   cur_time.Format("%Y-%m-%d"); 
		break;   
	case   VT_BOOL:   
		strValue.Format("%d",var.boolVal   );   
		break;   
	default:     
		strValue   =   _T("");   
		break;   
	}   
	return   strValue;   
}


void CWeighToolView::OnChangeEditCartonNum() 
{
	UpdateData();
	
	if(m_csCartonNum.Right(2)=="\r\n")
	{
		m_csCartonNum.Replace("\r\n","");
		UpdateData(FALSE);

		//检测卡通箱号
		if(!CheckCartonNum())
		{
			m_ctrCartonNum.SetFocus();
			m_ctrCartonNum.SetSel(0,-1);
			ShowCalResult(FAIL);
			return;
		}
		
		ShowCalResult(PASS);
		
		if(!this->Load_Current_Product_Count())
		{
			InsertListInfo("To obtain the number of current orders has failed",2);
			return;
		}
		InsertListInfo("Obtain the number of current orders has been successful",0);
		this->ClearUI();
		this->UpdateCount(this->m_csCurrentCount);	
	}
}


BOOL CWeighToolView::CheckCartonNum()
{
	_variant_t var;
	CString sql,temp;
	CString	CartonNameStatictemp,CartonNametemp;
	int CartonNameStaticLength;
	CStringArray m_csaIMEI1;

	//获取扫描卡通箱号的固定位及流水位
	CartonNameStaticLength = strlen(csCartonNameStatic);
	CartonNameStatictemp = m_csCartonNum.Left(CartonNameStaticLength);
	CartonNametemp = m_csCartonNum.Right(m_csCartonNum.GetLength() - CartonNameStaticLength);

	//检查扫描录入的卡通箱号固定位是否正确
	if(CartonNameStatictemp != csCartonNameStatic)
	{
		InsertListInfo("A fixed number of cartoon scanning input and database records is not consistent...",2);
		this->m_ctrCartonNum.SetSel(0,-1);
		return FALSE;
	}
	else
	{
		InsertListInfo("The cartoon number fixed bit detection...",0);
	}

	//检查卡通箱号及是否包装完毕并封箱
	try
	{
		sql.Format("select *  from %s_%s_CartonBox where CartonName='%s' and Status=2",this->m_csCurrentProduct,m_csCurrentOrder,CartonNametemp);
		if(((CWeighToolApp *)AfxGetApp())->m_pRst->State)
			((CWeighToolApp *)AfxGetApp())->m_pRst->Close();
		((CWeighToolApp *)AfxGetApp())->m_pRst=((CWeighToolApp *)AfxGetApp())->m_pConnection->Execute((_bstr_t)sql,NULL,adCmdText);
		if(!((CWeighToolApp *)AfxGetApp())->m_pRst->adoEOF)
		{
			//卡通箱号存在于数据库中
			InsertListInfo("The cartoon box number exists in the database...",0);
			m_checkCartonDone=TRUE;
		}
		else
		{
			//数据库内无记录
			InsertListInfo("The number is not in the cartoon cartoon box in the database or not sealing...",2);
			m_checkCartonDone=FALSE;
		}

		if(((CWeighToolApp *)AfxGetApp())->m_pRst->State)
		((CWeighToolApp *)AfxGetApp())->m_pRst->Close();
	}
	catch(_com_error e)
	{
		InsertListInfo("Unknown error...",2);
		this->m_ctrCartonNum.SetSel(0,-1);
		return FALSE;
	}

	//获取卡通箱中IMEI信息
	try
	{
		sql.Format("select *  from %s_%s_ColorBox where CartonName='%s' and Enable=1",this->m_csCurrentProduct,m_csCurrentOrder,CartonNametemp);
		if(((CWeighToolApp *)AfxGetApp())->m_pRst->State)
			((CWeighToolApp *)AfxGetApp())->m_pRst->Close();
		((CWeighToolApp *)AfxGetApp())->m_pRst=((CWeighToolApp *)AfxGetApp())->m_pConnection->Execute((_bstr_t)sql,NULL,adCmdText);
		while(!((CWeighToolApp *)AfxGetApp())->m_pRst->adoEOF)
		{
			var=((CWeighToolApp *)AfxGetApp())->m_pRst->GetCollect("IMEI1");
			temp=VariantToCString(var);
			m_csaIMEI1.Add(temp);

			((CWeighToolApp *)AfxGetApp())->m_pRst->MoveNext();
		}

		if(((CWeighToolApp *)AfxGetApp())->m_pRst->State)
		((CWeighToolApp *)AfxGetApp())->m_pRst->Close();
	}
	catch(_com_error e)
	{
		InsertListInfo("Unknown error...",2);
		this->m_ctrCartonNum.SetSel(0,-1);
		return FALSE;
	}

	//如果卡通箱表中有记录，写入重量数据
	if(m_checkCartonDone)
	{
		try
		{
			//判断重量
			if(this->m_dWeigh >this->m_dHighWeigh || this->m_dWeigh < this->m_dLowWeigh)
			{
				InsertListInfo("Weight does not conform to the specification...",2);
				return FALSE;
			}
					
			sql.Format("UPDATE %s_%s_CartonBox SET CartonWeigh=%.1f where CartonName='%s'",this->m_csCurrentProduct,this->m_csCurrentOrder,this->m_dWeigh,CartonNametemp);
			if(((CWeighToolApp *)AfxGetApp())->m_pRst->State)
				((CWeighToolApp *)AfxGetApp())->m_pRst->Close();
			((CWeighToolApp *)AfxGetApp())->m_pConnection->Execute((_bstr_t)sql,NULL,adCmdText);
			if(((CWeighToolApp *)AfxGetApp())->m_pRst->State)
				((CWeighToolApp *)AfxGetApp())->m_pRst->Close();
			
			for (int i=0; i<m_csaIMEI1.GetSize(); i++)
			{
				sql.Format("UPDATE %s_%s_ProductionLine SET CartonWeighTool='%s' where IMEI1='%s'", \
					(char*)(LPCTSTR)this->m_csCurrentProduct,this->m_csCurrentOrder, \
					this->m_csCurrentLine,m_csaIMEI1[i]);

					if(((CWeighToolApp *)AfxGetApp())->m_pRst->State)
						((CWeighToolApp *)AfxGetApp())->m_pRst->Close();
					((CWeighToolApp *)AfxGetApp())->m_pConnection->Execute((_bstr_t)sql,NULL,adCmdText);
					if(((CWeighToolApp *)AfxGetApp())->m_pRst->State)
						((CWeighToolApp *)AfxGetApp())->m_pRst->Close();
			}
		}
		catch(_com_error e)
		{
			InsertListInfo("Database updating the weight parameter of cartoon box...",2);
			return FALSE;
		}

		temp.Format("Weighing success, weight=%.1fg,Database update cartoon box weight success...",this->m_dWeigh);
		InsertListInfo(temp,0);
	}
	else
	{
		return FALSE;
	}

	return TRUE;
}


void CWeighToolView::ClearUI()
{
	this->m_csCartonNum.Empty();
	UpdateData(FALSE);
	this->m_ctrCartonNum.SetFocus();
}


int CWeighToolView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CFormView::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	CSplashWnd::ShowSplashScreen(this);
	
	return 0;
}


void CWeighToolView::ShowCalResult(int state)
{	
	switch(state) 
	{
	case FAIL:
		this->m_cResult.SetTextColor(RGB(255,0,0));
		this->m_cResult.SetWindowText("FAIL");
		break;
	case PASS:
		this->m_cResult.SetTextColor(RGB(0,128,0));
		this->m_cResult.SetWindowText("PASS");
		break;
	default:
		break;
	}
}


bool CWeighToolView::GetProductOrderInfo()
{
	CString temp;
	_variant_t var;
	CString sql;
	
	try
	{
		sql.Format("select *  from ProductList where Enable=1 and OrderName='%s'",m_csCurrentOrder);
		
		if(((CWeighToolApp *)AfxGetApp())->m_pRst->State)
			((CWeighToolApp *)AfxGetApp())->m_pRst->Close();
		((CWeighToolApp *)AfxGetApp())->m_pRst=((CWeighToolApp *)AfxGetApp())->m_pConnection->Execute((_bstr_t)sql,NULL,adCmdText);
		if(!((CWeighToolApp *)AfxGetApp())->m_pRst->adoEOF)
		{
			var = ((CWeighToolApp *)AfxGetApp())->m_pRst->GetCollect("ProductName");
			temp=VariantToCString(var);
			this->m_csCurrentProduct= temp;

			var = ((CWeighToolApp *)AfxGetApp())->m_pRst->GetCollect("Color");
			temp=VariantToCString(var);
			this->m_csCurrentColor= temp;

			var = ((CWeighToolApp *)AfxGetApp())->m_pRst->GetCollect("CartonNameStatic");
			temp=VariantToCString(var);
			this->csCartonNameStatic= temp;

			var = ((CWeighToolApp *)AfxGetApp())->m_pRst->GetCollect("CartonNameSNLength");
			this->iCartonNameSNLength=var.intVal;

			var = ((CWeighToolApp *)AfxGetApp())->m_pRst->GetCollect("CartonNameBegin");
			temp=VariantToCString(var);
			this->csCartonNameBegin= temp;
			
			var = ((CWeighToolApp *)AfxGetApp())->m_pRst->GetCollect("CartonNameEnd");
			temp=VariantToCString(var);
			this->csCartonNameEnd= temp;
		}
		else
		{
			return  FALSE;
		}
		if(((CWeighToolApp *)AfxGetApp())->m_pRst->State)
			((CWeighToolApp *)AfxGetApp())->m_pRst->Close();
	}
	catch(_com_error e)
	{
		return FALSE;
	}	
	
	return TRUE;
}


bool CWeighToolView::Load_Current_Product_Count()
{
	CString temp,states;
	_variant_t var;
	CString sql;

	sql.Format("select count(*) as COUNT from %s_%s_CartonBox WHERE (CartonWeigh<>'' or CartonWeigh IS NOT NULL)",(char*)(LPCTSTR)this->m_csCurrentProduct,this->m_csCurrentOrder);

	try
	{
		if(((CWeighToolApp *)AfxGetApp())->m_pRst->State)
			((CWeighToolApp *)AfxGetApp())->m_pRst->Close();
		((CWeighToolApp *)AfxGetApp())->m_pRst=((CWeighToolApp *)AfxGetApp())->m_pConnection->Execute((_bstr_t)sql,NULL,adCmdText);
		
		if(!((CWeighToolApp *)AfxGetApp())->m_pRst->adoEOF)
		{			
			var = ((CWeighToolApp *)AfxGetApp())->m_pRst->GetCollect("COUNT");
			temp=VariantToCString(var);
			this->m_csCurrentCount= temp;
		}
		if(((CWeighToolApp *)AfxGetApp())->m_pRst->State)
			((CWeighToolApp *)AfxGetApp())->m_pRst->Close();
	}
	catch(_com_error e)
	{
		return FALSE;
	}
	
	return TRUE;
}


void CWeighToolView::UpdateCount(CString strCount)
{
	CString temp;
	temp="Quantity:"+strCount;
	this->m_cCurrentCount.SetWindowText(temp);
	UpdateData(FALSE);
}


BOOL CWeighToolView::GetSetting()
{
	CString temp;
	_variant_t var;
	CString sql;

	try
	{
		sql.Format("select * from SettingList where Enable=1 and ProductName='%s' and OrderName='%s'",this->m_csCurrentProduct,this->m_csCurrentOrder);
		if(((CWeighToolApp *)AfxGetApp())->m_pRst->State)
			((CWeighToolApp *)AfxGetApp())->m_pRst->Close();
		((CWeighToolApp *)AfxGetApp())->m_pRst=((CWeighToolApp *)AfxGetApp())->m_pConnection->Execute((_bstr_t)sql,NULL,adCmdText);
		
		if(!((CWeighToolApp *)AfxGetApp())->m_pRst->adoEOF)
		{
			var = ((CWeighToolApp *)AfxGetApp())->m_pRst->GetCollect("CartonBoxLowWeigh");
			this->m_dLowWeigh=atof((_bstr_t)var);

			var = ((CWeighToolApp *)AfxGetApp())->m_pRst->GetCollect("CartonBoxHighWeigh");
			this->m_dHighWeigh=atof((_bstr_t)var);
		}
		else
		{
			return FALSE;
		}
		if(((CWeighToolApp *)AfxGetApp())->m_pRst->State)
			((CWeighToolApp *)AfxGetApp())->m_pRst->Close();
	}
	catch(_com_error e)
	{
		return FALSE;
	}

	return TRUE;
}

void CWeighToolView::OnBtnSetting() 
{
	CPassword dlg;
	dlg.ilevel = 1; //需超级权限验证
	if(dlg.DoModal()!=IDOK)
	{
		return;
	}

	CWeighSetting pdlg;
	pdlg.m_csProduct = m_csCurrentProduct;
	pdlg.m_csOrder = m_csCurrentOrder;
	if(pdlg.DoModal()!=IDOK)
	{
		return;
	}
}

void CWeighToolView::OnBtnRefresh() 
{
	CString temp;
	_variant_t var;
	CString sql;

	try
	{
		sql.Format("select * from SettingList where Enable=1 and ProductName='%s' and OrderName='%s'",this->m_csCurrentProduct,this->m_csCurrentOrder);
		if(((CWeighToolApp *)AfxGetApp())->m_pRst->State)
			((CWeighToolApp *)AfxGetApp())->m_pRst->Close();
		((CWeighToolApp *)AfxGetApp())->m_pRst=((CWeighToolApp *)AfxGetApp())->m_pConnection->Execute((_bstr_t)sql,NULL,adCmdText);
		
		if(!((CWeighToolApp *)AfxGetApp())->m_pRst->adoEOF)
		{
			var = ((CWeighToolApp *)AfxGetApp())->m_pRst->GetCollect("CartonBoxLowWeigh");
			this->m_dLowWeigh=atof((_bstr_t)var);

			var = ((CWeighToolApp *)AfxGetApp())->m_pRst->GetCollect("CartonBoxHighWeigh");
			this->m_dHighWeigh=atof((_bstr_t)var);
		}
		else
		{
			return;
		}
		if(((CWeighToolApp *)AfxGetApp())->m_pRst->State)
			((CWeighToolApp *)AfxGetApp())->m_pRst->Close();
	}
	catch(_com_error e)
	{
		InsertListInfo("Get the box weight range failed...",2);
		return;
	}

	UpdateData(FALSE);
}
