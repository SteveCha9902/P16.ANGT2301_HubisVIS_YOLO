
// VisionSPCDlg.cpp: 구현 파일
//

#include "pch.h"
#include "framework.h"
#include "VisionSPC.h"
#include "VisionSPCDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CVisionSPCDlg 대화 상자
CVisionSPCDlg::CVisionSPCDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_VISIONSPC_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CVisionSPCDlg::~CVisionSPCDlg()
{
	delete m_pImageDisplay;

	for (long i = 0; i < (long)m_vecpyRed.size(); i++)
	{
		BYTE* pData = m_vecpyRed[i];
		delete[] pData;
	}
	m_vecpyRed.clear();

	for (long i = 0; i < (long)m_vecpyGreen.size(); i++)
	{
		BYTE* pData = m_vecpyGreen[i];
		delete[] pData;
	}
	m_vecpyGreen.clear();

	for (long i = 0; i < (long)m_vecpyBlue.size(); i++)
	{
		BYTE* pData = m_vecpyBlue[i];
		delete[] pData;
	}
	m_vecpyBlue.clear();

}

void CVisionSPCDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_FILE_LIST, m_listFile);
	DDX_Control(pDX, IDC_LIST_RESULT_LIST, m_listResult);
	DDX_Control(pDX, IDC_DATETIMEPICKER_START, m_date_Start);
	DDX_Control(pDX, IDC_DATETIMEPICKER_END, m_date_End);
	DDX_Control(pDX, IDC_TAB_IMAGE, tabImage);
}

BEGIN_MESSAGE_MAP(CVisionSPCDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_IMAGE, &CVisionSPCDlg::OnTcnSelchangeTabImage)
	ON_NOTIFY(NM_CLICK, IDC_LIST_FILE_LIST, &CVisionSPCDlg::OnNMClickListFileList)
	ON_BN_CLICKED(IDC_BTN_SEARCH, &CVisionSPCDlg::OnBnClickedBtnSearch)
	ON_BN_CLICKED(IDC_BTN_OPEN, &CVisionSPCDlg::OnBnClickedBtnOpen)
END_MESSAGE_MAP()


// CVisionSPCDlg 메시지 처리기

BOOL CVisionSPCDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 시스템 메뉴에 "정보..." 메뉴 항목을 추가합니다.

	// IDM_ABOUTBOX는 시스템 명령 범위에 있어야 합니다.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 이 대화 상자의 아이콘을 설정합니다.  응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	// TODO: 여기에 추가 초기화 작업을 추가합니다.
	CString strCurPath, strConfig;
	GetModuleFileName(nullptr, strCurPath.GetBuffer(_MAX_PATH + 1), _MAX_PATH);
	strCurPath.ReleaseBuffer();
	m_strPath = strCurPath.Left(strCurPath.ReverseFind(_T('\\')));
	strConfig.Format(_T("%s\\Config"), m_strPath);
	CreateDirectory(strConfig, nullptr);

	::g_ConfigManager.SetCurPath(m_strPath);

	// config.ini 파일을 읽어온다.
	::g_ConfigManager.InitParameters();
	::g_ConfigManager.LoadConfig();
	::g_ConfigManager.SaveConfig();

	m_pImageDisplay = new CImageDisplay;
	m_pImageDisplay->Initialize(IDC_STATIC_IMAGE, this, RGB(50, 50, 50));
	m_pImageDisplay->SetCrossLineColor(RGB(255, 0, 0));
	m_pImageDisplay->SetCustomLineColor(RGB(255, 0, 0));

	InitFileListControl();
	InitResultListControl();
	InitTabImage();
	
	SetFileListControl();
	SetResultListControl();

	m_nCurrIndex = -1;
	m_nImageNum = 0;
	m_nImageWidth = 0;
	m_nImageHeight = 0;

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

void CVisionSPCDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	CDialogEx::OnSysCommand(nID, lParam);
}

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다.  문서/뷰 모델을 사용하는 MFC 응용 프로그램의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void CVisionSPCDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CVisionSPCDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CVisionSPCDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case VK_RETURN:
		case VK_ESCAPE:
			return TRUE;
		}
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}

void CVisionSPCDlg::InitFileListControl()
{
	m_listFile.SetExtendedStyle(m_listFile.GetExtendedStyle() | LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);
	m_listFile.InsertColumn(0, _T("No."), LVCFMT_CENTER, 40);
	m_listFile.InsertColumn(1, _T("Date"), LVCFMT_CENTER, 100);
	//m_listFile.InsertColumn(2, _T("Model"), LVCFMT_CENTER, 120);
	m_listFile.InsertColumn(2, _T("Barcode"), LVCFMT_CENTER, 150);
}

void CVisionSPCDlg::InitResultListControl()
{
	m_listResult.SetExtendedStyle(m_listResult.GetExtendedStyle() | LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);
	m_listResult.InsertColumn(0, _T("No."), LVCFMT_CENTER, 40);
	m_listResult.InsertColumn(1, _T("Result_0"), LVCFMT_CENTER, 100);
	m_listResult.InsertColumn(2, _T("Result_1"), LVCFMT_CENTER, 100);
	m_listResult.InsertColumn(3, _T("Result_2"), LVCFMT_CENTER, 100);
}

void CVisionSPCDlg::InitTabImage()
{
	tabImage.InsertItem(0, _T("Image_0"));
	tabImage.InsertItem(1, _T("Image_1"));
	tabImage.InsertItem(2, _T("Image_2"));
}

void CVisionSPCDlg::SetFileListControl()
{
	CString strTemp;
	CString strDate;
	std::vector<CString> file;

	m_listFile.DeleteAllItems();

	for (long idx = 0; idx < m_vecstrFileList.size(); idx++)
	{
		strTemp.Format(_T("%d"),idx + 1);
		m_listFile.InsertItem(idx, strTemp);

		//File명에 따라서 변경될 수 있음
		file.clear();
		DivideToken(file, m_vecstrFileList[idx], _T("_"));

		//Date
		m_listFile.SetItemText(idx, 1, file[1] + file[2]);
		//Barcode
		m_listFile.SetItemText(idx, 2, file[0]);

		//Model 추가될경우 작성

	}
}

void CVisionSPCDlg::SetResultListControl()
{
	CString strTemp;
	std::vector<CString> result;
	m_listResult.DeleteAllItems();

	for (long idx = 0; idx < m_vecstrResultList.size(); idx++)
	{
		strTemp.Format(_T("%d"), idx + 1);
		m_listResult.InsertItem(idx, strTemp);

		//CSV 내부 내용에 따라서 변경될 수 있음
		result.clear();
		DivideToken(result, m_vecstrResultList[idx], _T(","));

		for (long token = 0; token < result.size(); token++)
		{
			if(token == result.size()-1) // 이미지 경로 위치 
			{
				m_vecstrImagePath.push_back(result[token]);
				continue;
			}

			m_listResult.SetItemText(idx, token + 1, result[token]);
		}
	}
}

void CVisionSPCDlg::SetTabImage()
{
	tabImage.DeleteAllItems();

	CString strTemp;
	for (long idx = 0; idx < m_nImageNum; idx++)
	{
		strTemp.Format(_T("Image_%d"), idx + 1);
		tabImage.InsertItem(idx, strTemp);
	}

	tabImage.SetCurSel(0);
}

void CVisionSPCDlg::UpdateImageDisplay()
{
	long nCurSel = tabImage.GetCurSel();

	if(m_vecpyRed.size() > 0)
	{
		m_pImageDisplay->SetImage(m_vecpyRed[nCurSel], m_vecpyGreen[nCurSel], m_vecpyBlue[nCurSel], m_nImageWidth, m_nImageHeight, m_nImageWidth);
	}
}

void CVisionSPCDlg::DoSearch(CString strPath)
{
	CFileFind find;

	CString fname;
	CString pname;

	BOOL res = find.FindFile((LPCTSTR)(strPath + "\\*.*"));

	while (res)
	{
		res = find.FindNextFile();
		fname = find.GetFileName();
		pname = find.GetFilePath();

		if (find.IsDirectory())
		{
			if (fname == "." || fname == "..")
				continue;

			for (long idx = 0; idx < m_vecstrDate.size(); idx++)
			{
				if (fname.Find(m_vecstrDate[idx]) != -1 /*&& fname.Find(m_strModel) != -1*/ && fname.Find(m_strBarcode) != -1 && fname.Find(_T("csv")) != -1)
				{
					m_vecstrFileList.push_back(fname);
				}
			}

			DoSearch(pname);
		}
		else
		{
			for (long idx = 0; idx < m_vecstrDate.size(); idx++)
			{
				if (fname.Find(m_vecstrDate[idx]) != -1 /*&& fname.Find(m_strModel) != -1*/ && fname.Find(m_strBarcode) != -1 && fname.Find(_T("csv")) != -1)
				{
					m_vecstrFileList.push_back(fname);
				}
			}
		}
	}
}

BOOL  CVisionSPCDlg::ResultFileOpen()
{
	if (m_nCurrIndex <= -1)
	{
		return FALSE;
	}

	CString strPath = ::g_ConfigManager.m_strResultDataSavePath + _T("\\")+ m_vecstrFileList[m_nCurrIndex];

	CFile pFile;
	if(pFile.Open(strPath, CFile::modeRead, NULL) == FALSE)
	{
		return FALSE;
	}

	//CSV 내부 파일 읽어오기
	CString source; 
	const int bufsize = STRING_MAX;
	char buf[bufsize];
	int count = pFile.Read(buf, bufsize);

	while (count > 0) 
	{
		buf[count] = 0;

		source += buf;

		count = pFile.Read(buf, bufsize);
	}

	source.Remove('\r');
	std::vector<CString> lines;
	DivideToken(lines, source, _T("\n"));

	m_vecstrResultList.clear();
	m_nImageNum = (long)lines.size() - 1 - 2; // 2개 row가 다른 데이터 이므로

	for (int row = 2; row < (int)lines.size(); row++) 
	{
		std::vector<CString> fields; 
		DivideToken(fields, lines[row], _T(","));

		if (row == 2)
		{
			for (int column = 0; column < (int)fields.size() - 1; column++)
			{
				m_listResult.DeleteColumn(column + 1);
				m_listResult.InsertColumn(column + 1, fields[column], LVCFMT_CENTER, 70);
			}
		}
		else
		{
			m_vecstrResultList.push_back(lines[row]);
		}
	}

	return TRUE;
}

void CVisionSPCDlg::DivideToken(std::vector<CString> &vecResult, const CString &strSource, CString strToken)
{
	long end = 0;
	for(long start = 0; start < strSource.GetLength(); start = end + 1)
	{
		end = strSource.Find(strToken, start);

		if (end < 0)
			end = strSource.GetLength();

		vecResult.push_back(strSource.Mid(start, end - start));
	}
}

BOOL CVisionSPCDlg::ResultImageOpen()
{
	if (m_vecstrImagePath.size() < 0)
	{
		return FALSE;
	}

	for (long idx = 0; idx < m_nImageNum; idx++)
	{
		CString strPath = ::g_ConfigManager.m_strResultDataSavePath + _T("\\");
		strPath += m_vecstrImagePath[idx];
		strPath += _T(".jpg");

		CImage* image = new CImage;
		image->Destroy();
		if (image->Load(strPath) != S_OK)
		{
			delete[] image;
			return FALSE;
		}

		m_nImageWidth = image->GetWidth();
		m_nImageHeight = image->GetHeight();

		BYTE* pbyImage = new BYTE[m_nImageWidth * m_nImageHeight * 3];
		memset(pbyImage, 0, m_nImageWidth * m_nImageHeight * 3);

		if (HUBSDK::ImageInterface::CImageToByteArray(image, pbyImage, m_nImageWidth, m_nImageHeight, 3) != FALSE)
		{
			return FALSE;
		}

		BYTE* red = new BYTE[m_nImageWidth * m_nImageHeight];
		BYTE* green = new BYTE[m_nImageWidth * m_nImageHeight];
		BYTE* blue = new BYTE[m_nImageWidth * m_nImageHeight];
		memset(red, 0, m_nImageWidth * m_nImageHeight);
		memset(green, 0, m_nImageWidth * m_nImageHeight);
		memset(blue, 0, m_nImageWidth * m_nImageHeight);

		if (HUBSDK::ImageProcessing::SplitRGB(pbyImage, m_nImageWidth, m_nImageHeight, red, green, blue) != retStatus::success)
		{
			return FALSE;
		}

		m_vecpyRed.push_back(red);
		m_vecpyGreen.push_back(green);
		m_vecpyBlue.push_back(blue);

		delete pbyImage;
	}

	return TRUE;
}

void CVisionSPCDlg::OnTcnSelchangeTabImage(NMHDR *pNMHDR, LRESULT *pResult)
{
	UpdateImageDisplay();

	*pResult = 0;
}


void CVisionSPCDlg::OnNMClickListFileList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	LPNMITEMACTIVATE pNMITEM = (LPNMITEMACTIVATE)pNMHDR;

	m_nCurrIndex = pNMITEM->iItem;

	*pResult = 0;
}


void CVisionSPCDlg::OnBnClickedBtnSearch()
{
	//검색 날짜를 문자형식으로 변경
	m_vecstrDate.clear();
	CTime StartTime;
	CTime EndTime;
	m_date_Start.GetTime(StartTime);
	m_date_End.GetTime(EndTime);

	CTimeSpan diffTime = EndTime - StartTime;

	long nDiff = (long)diffTime.GetDays();

	CString strTemp;
	for (long idx = 0; idx <= nDiff; idx++)
	{
		strTemp = StartTime.Format(_T("%Y_%m%d"));
		m_vecstrDate.push_back(strTemp);
		StartTime += CTimeSpan(1, 0, 0, 0);
	}

	//GetDlgItemText(IDC_EDIT_MODEL, m_strModel);
	GetDlgItemText(IDC_EDIT_BARCODE, m_strBarcode);

	//if (m_strModel.IsEmpty() == TRUE)
	//	m_strModel = _T(".");

	if (m_strBarcode.IsEmpty() == TRUE)
		m_strBarcode = _T("BarcodeEmpty");

	CString strPath = ::g_ConfigManager.m_strResultDataSavePath;

	m_vecstrFileList.clear();

	DoSearch(strPath);

	SetFileListControl();
}


void CVisionSPCDlg::OnBnClickedBtnOpen()
{
	ResultFileOpen();
	SetTabImage();
	SetResultListControl();

	ResultImageOpen();
	UpdateImageDisplay();
}