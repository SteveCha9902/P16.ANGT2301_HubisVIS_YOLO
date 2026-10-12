// DlgAlgoBlob.cpp: 구현 파일
//

#include "pch.h"
#include "VisionMain.h"
#include "DlgAlgoBlob.h"
#include "afxdialogex.h"

#include "DlgSetup.h"
#include "DlgCalibration.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CDlgAlgoBlob 대화 상자

IMPLEMENT_DYNAMIC(CDlgAlgoBlob, CDialogEx)

CDlgAlgoBlob::CDlgAlgoBlob(CDlgSetup* pParent, CDlgCalibration* pCalibration /*=nullptr*/)
	: CDialogEx(IDD_DLG_ALGO_BLOB, pParent)
	, m_pParent(pParent)
	, m_pCalibration(pCalibration)
{
	if (m_pParent != nullptr)
	{
		m_pImageDisplay = pParent->m_pImageDisplay;
		m_nImageWidth = pParent->m_nImageWidth;
		m_nImageHeight = pParent->m_nImageHeight;
		m_nImageSize = pParent->m_nImageSize;
		m_nCurItemIndex = pParent->m_nLastestSelectedInspItemIndex;
	}
	else
	{
		m_pImageDisplay = m_pCalibration->m_pImageDisplay;
		m_nImageWidth = m_pCalibration->m_nImageWidth;
		m_nImageHeight = m_pCalibration->m_nImageHeight;
		m_nImageSize = m_pCalibration->m_nImageSize;
		m_nCurItemIndex = 0;
	}
}

CDlgAlgoBlob::~CDlgAlgoBlob()
{
}

void CDlgAlgoBlob::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_BLOBLIST, m_listBlobInfo);
	DDX_Control(pDX, IDC_MFCPROPERTYGRID_BLOB, m_propertyGridBlob);
}

BEGIN_MESSAGE_MAP(CDlgAlgoBlob, CDialogEx)
	ON_BN_CLICKED(IDC_BTN_UPDATEBLOBLIST, &CDlgAlgoBlob::OnBnClickedBtnUpdatebloblist)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_BLOBLIST, &CDlgAlgoBlob::OnLvnItemchangedListBloblist)
	ON_REGISTERED_MESSAGE(AFX_WM_PROPERTY_CHANGED, &CDlgAlgoBlob::OnAfxWmPropertyChanged)
END_MESSAGE_MAP()


// CDlgAlgoBlob 메시지 처리기


BOOL CDlgAlgoBlob::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_listBlobInfo.SetExtendedStyle(m_listBlobInfo.GetExtendedStyle() | LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);
	m_listBlobInfo.InsertColumn(0, _T("No."), LVCFMT_CENTER, 40);
	m_listBlobInfo.InsertColumn(1, _T("Area(px)"), LVCFMT_CENTER, 80);
	m_listBlobInfo.InsertColumn(2, _T("Width(px)"), LVCFMT_CENTER, 80);
	m_listBlobInfo.InsertColumn(3, _T("Height(px)"), LVCFMT_CENTER, 80);
	m_listBlobInfo.InsertColumn(4, _T("AspectRatio"), LVCFMT_CENTER, 80);

	InitPropertyGrid();
	//UpdateUI();

	return TRUE;
}

void CDlgAlgoBlob::UpdateBlobList()
{
	CString strNo, strTemp;

	m_pImageDisplay->Overlay_RemoveAll();

	auto algoSequence = ::g_RecipeManager.m_vecsInspItem[m_nCurItemIndex].algoSequence;

	std::vector<BlobInfo> blobInfo;

	if(algoSequence == (long)ALGORITHM_SEQUENCE::EDGE_DETECT ||
		algoSequence == (long)ALGORITHM_SEQUENCE::DETECT_BLOB)
	{
		blobInfo = ::g_InspectionManager.m_pInspection->m_vecBlobInfoList;
	}
	else if(algoSequence == (long)ALGORITHM_SEQUENCE::DETECT_BLOB2)
	{
		auto curAlgoritm = m_pParent->m_tabAlgorithm.GetCurSel();

		if(curAlgoritm == (long)DETECT_BLOB2_SEQUENCE::BLOB)
		{
			blobInfo = ::g_InspectionManager.m_pInspection->m_vecBlobInfoList;
		}
		else if(curAlgoritm == (long)DETECT_BLOB2_SEQUENCE::BLOB2)
		{
		blobInfo = ::g_InspectionManager.m_pInspection->m_vecBlobInfoList2;
	}
	}

	// List 에 있는 내용을 모두 지운다.
	m_listBlobInfo.DeleteAllItems();

	// Blob List에 Blob 정보 보여주기
	for(long blob = 0; blob < blobInfo.size(); blob++)
	{
		auto info = blobInfo[blob];

		m_listBlobInfo.InsertItem(blob, _T(""));

		strNo.Format(_T("%d"), blob + 1);
		m_listBlobInfo.SetItemText(blob, 0, strNo);

		strTemp.Format(_T("%d"), info.m_area);
		m_listBlobInfo.SetItemText(blob, 1, strTemp);
		strTemp.Format(_T("%d"), info.m_roi.Width());
		m_listBlobInfo.SetItemText(blob, 2, strTemp);
		strTemp.Format(_T("%d"), info.m_roi.Height());
		m_listBlobInfo.SetItemText(blob, 3, strTemp);
		strTemp.Format(_T("%.3f"), (float)info.m_roi.Width() / (float)info.m_roi.Height());
		m_listBlobInfo.SetItemText(blob, 4, strTemp);
	}
}

void CDlgAlgoBlob::ClearBlobList()
{
	m_listBlobInfo.DeleteAllItems();
}

void CDlgAlgoBlob::OnBnClickedBtnUpdatebloblist()
{
	HUBSDK::ThreadedProgress progressX(_T("Processing now..."));

	auto inspItem = ::g_RecipeManager.m_vecsInspItem[m_nCurItemIndex];
	auto algoSequence = ::g_RecipeManager.m_vecsInspItem[m_nCurItemIndex].algoSequence;

	if (m_pParent != nullptr)
	{
		if(algoSequence == (long)ALGORITHM_SEQUENCE::EDGE_DETECT ||
			algoSequence == (long)ALGORITHM_SEQUENCE::DETECT_BLOB)
		{
			m_pParent->UpdateAlgoBlob();
		}
		else if(algoSequence == (long)ALGORITHM_SEQUENCE::DETECT_BLOB2)
		{
			auto curAlgoritm = m_pParent->m_tabAlgorithm.GetCurSel();

			if(curAlgoritm == (long)DETECT_BLOB2_SEQUENCE::BLOB)
			{
		m_pParent->UpdateAlgoBlob();
			}
			else if(curAlgoritm == (long)DETECT_BLOB2_SEQUENCE::BLOB2)
			{
				m_pParent->UpdateAlgoBlob2();
			}
		}
	}
	else
	{
		m_pCalibration->UpdateAlgoBlob();
	}

	UpdateBlobList();

	if (m_pParent != nullptr)
	{
		if(algoSequence == (long)ALGORITHM_SEQUENCE::EDGE_DETECT ||
			algoSequence == (long)ALGORITHM_SEQUENCE::DETECT_BLOB)
		{
			m_pParent->ShowResultOverlay(inspItem.algoSequence, (long)DETECT_BLOB_SEQUENCE::BLOB);
		}
		else if(algoSequence == (long)ALGORITHM_SEQUENCE::DETECT_BLOB2)
		{
			auto curAlgoritm = m_pParent->m_tabAlgorithm.GetCurSel();

			m_pParent->ShowResultOverlay(inspItem.algoSequence, curAlgoritm);
		}
	}
	else
	{
		m_pCalibration->ShowResultOverlay((long)DETECT_BLOB_SEQUENCE::BLOB);
	}

}

void CDlgAlgoBlob::OnLvnItemchangedListBloblist(NMHDR * pNMHDR, LRESULT * pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	auto algoSequence = ::g_RecipeManager.m_vecsInspItem[m_nCurItemIndex].algoSequence;

	std::vector<BlobInfo> blobInfo;

	if(m_pParent != nullptr)
	{
		if(algoSequence == (long)ALGORITHM_SEQUENCE::EDGE_DETECT ||
			algoSequence == (long)ALGORITHM_SEQUENCE::DETECT_BLOB)
		{
			m_pParent->UpdateAlgoBlob();
			blobInfo = ::g_InspectionManager.m_pInspection->m_vecBlobInfoList;
		}
		else if(algoSequence == (long)ALGORITHM_SEQUENCE::DETECT_BLOB2)
		{
			auto curAlgoritm = m_pParent->m_tabAlgorithm.GetCurSel();

			if(curAlgoritm == (long)DETECT_BLOB2_SEQUENCE::BLOB)
			{
				m_pParent->UpdateAlgoBlob();
				blobInfo = ::g_InspectionManager.m_pInspection->m_vecBlobInfoList;
			}
			else if(curAlgoritm == (long)DETECT_BLOB2_SEQUENCE::BLOB2)
			{
				m_pParent->UpdateAlgoBlob2();
				blobInfo = ::g_InspectionManager.m_pInspection->m_vecBlobInfoList2;
			}
		}
	}
	else
	{
		m_pCalibration->UpdateAlgoBlob();
	}

	if ((pNMLV->uNewState & LVIS_SELECTED) == LVIS_SELECTED)
	{
		int nItem = pNMLV->iItem;

		auto inspItem = ::g_RecipeManager.m_vecsInspItem[m_nCurItemIndex];

		if (m_pParent != nullptr)
			m_pParent->ShowResultOverlay(inspItem.algoSequence, (long)DETECT_BLOB_SEQUENCE::BLOB);
		else
			m_pCalibration->ShowResultOverlay((long)DETECT_BLOB_SEQUENCE::BLOB);

		m_pImageDisplay->Overlay_AddRectangle(blobInfo[nItem].m_roi, RGB(255, 0, 0));

		m_pImageDisplay->Overlay_Show(TRUE);
	}

	*pResult = 0;
}

LRESULT CDlgAlgoBlob::OnAfxWmPropertyChanged(WPARAM wParam, LPARAM lParam)
{
	CMFCPropertyGridProperty *pProp = (CMFCPropertyGridProperty*)lParam;

	if(pProp == nullptr)
		return 0;

	CString strTemp;
	CString str = pProp->GetName();
	_variant_t value = pProp->GetValue();
	SBlobParams* params = nullptr;
	
	auto algoSequence = ::g_RecipeManager.m_vecsInspItem[m_nCurItemIndex].algoSequence;

	if (m_pParent != nullptr)
	{
		m_nCurItemIndex = m_pParent->GetItemIndex();

		if(algoSequence == (long)ALGORITHM_SEQUENCE::EDGE_DETECT ||
			algoSequence == (long)ALGORITHM_SEQUENCE::DETECT_BLOB)
		{
			params = &::g_RecipeManager.m_vecsInspItem[m_nCurItemIndex].sBlob;
		}
		else if(algoSequence == (long)ALGORITHM_SEQUENCE::DETECT_BLOB2)
		{
			auto curAlgoritm = m_pParent->m_tabAlgorithm.GetCurSel();

			if(curAlgoritm == (long)DETECT_BLOB2_SEQUENCE::BLOB)
			{
				params = &::g_RecipeManager.m_vecsInspItem[m_nCurItemIndex].sBlob;
			}
			else if(curAlgoritm == (long)DETECT_BLOB2_SEQUENCE::BLOB2)
			{
				params = &::g_RecipeManager.m_vecsInspItem[m_nCurItemIndex].sBlob2;
			}
		}
	}
	else
	{
		params = &m_pCalibration->m_sBlobParams;
	}

	if (params == NULL) return 0;

	if(str == _T("Deflate Left"))
	{
		params->nDeflateLeft = (long)value;
	}
	else if(str == _T("Deflate Top"))
	{
		params->nDeflateTop = (long)value;
	}
	else if(str == _T("Deflate Right"))
	{
		params->nDeflateRight = (long)value;
	}
	else if(str == _T("Deflate Bottom"))
	{
		params->nDeflateBottom = (long)value;
	}
	else if(str == _T("Min Blob Size"))
	{
		params->nMinBlobSize = (long)value;
	}
	else if(str == _T("Max Blob Count"))
	{
		params->nMaxBlobCount = (long)value;
	}
	else if(str == _T("Blob Merge"))
	{
		params->bBlobMerge = (CString)value == _T("TRUE") ? TRUE : FALSE;
	}
	else if(str == _T("Merge Distance"))
	{
		params->nBlobMergeDist = (long)value;
	}

	return 0;
}

void CDlgAlgoBlob::InitPropertyGrid()
{
	CString strTemp;
	SBlobParams params;
	params.Init();

	m_propertyGridBlob.RemoveAll();

	// Header Column Size 설정
	HDITEM item;
	item.cxy = 200;  // Size 
	item.mask = HDI_WIDTH;
	m_propertyGridBlob.GetHeaderCtrl().SetItem(0, &HDITEM(item));

	auto algoSequence = ::g_RecipeManager.m_vecsInspItem[m_nCurItemIndex].algoSequence;

	if (m_pParent != nullptr)
	{
		m_nCurItemIndex = m_pParent->GetItemIndex();

		if(algoSequence == (long)ALGORITHM_SEQUENCE::EDGE_DETECT ||
			algoSequence == (long)ALGORITHM_SEQUENCE::DETECT_BLOB)
		{
			params = ::g_RecipeManager.m_vecsInspItem[m_nCurItemIndex].sBlob;
		}
		else if(algoSequence == (long)ALGORITHM_SEQUENCE::DETECT_BLOB2)
		{
			auto curAlgoritm = m_pParent->m_tabAlgorithm.GetCurSel();

			if(curAlgoritm == (long)DETECT_BLOB2_SEQUENCE::BLOB)
			{
				params = ::g_RecipeManager.m_vecsInspItem[m_nCurItemIndex].sBlob;
			}
			else if(curAlgoritm == (long)DETECT_BLOB2_SEQUENCE::BLOB2)
			{
			params = ::g_RecipeManager.m_vecsInspItem[m_nCurItemIndex].sBlob2;
		}
	}
	}
	else
	{
		params = m_pCalibration->m_sBlobParams;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Blob ROI
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	CMFCPropertyGridProperty* pGroupBlobROI = new CMFCPropertyGridProperty(_T("Blob ROI"));
	m_propertyGridBlob.AddProperty(pGroupBlobROI);

	CMFCPropertyGridProperty* pGroupBlobROI_DeflateLeft = new CMFCPropertyGridProperty(_T("Deflate Left"), params.nDeflateLeft, _T(""));
	pGroupBlobROI->AddSubItem(pGroupBlobROI_DeflateLeft);

	CMFCPropertyGridProperty* pGroupBlobROI_DeflateTop = new CMFCPropertyGridProperty(_T("Deflate Top"), params.nDeflateTop, _T(""));
	pGroupBlobROI->AddSubItem(pGroupBlobROI_DeflateTop);

	CMFCPropertyGridProperty* pGroupBlobROI_DeflateRight = new CMFCPropertyGridProperty(_T("Deflate Right"), params.nDeflateRight, _T(""));
	pGroupBlobROI->AddSubItem(pGroupBlobROI_DeflateRight);

	CMFCPropertyGridProperty* pGroupBlobROI_DeflateBottom = new CMFCPropertyGridProperty(_T("Deflate Bottom"), params.nDeflateBottom, _T(""));
	pGroupBlobROI->AddSubItem(pGroupBlobROI_DeflateBottom);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Blob
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	CMFCPropertyGridProperty* pGroupBlob = new CMFCPropertyGridProperty(_T("Blob"));
	m_propertyGridBlob.AddProperty(pGroupBlob);

	CMFCPropertyGridProperty* pGroupBlob_MinBlobSize = new CMFCPropertyGridProperty(_T("Min Blob Size"), params.nMinBlobSize, _T(""));
	pGroupBlob->AddSubItem(pGroupBlob_MinBlobSize);

	CMFCPropertyGridProperty* pGroupBlob_MaxBlobCount = new CMFCPropertyGridProperty(_T("Max Blob Count"), params.nMaxBlobCount, _T(""));
	pGroupBlob->AddSubItem(pGroupBlob_MaxBlobCount);

	strTemp = params.bBlobMerge == TRUE ? _T("TRUE") : _T("FALSE");
	CMFCPropertyGridProperty* pGroupBlob_Merge = new CMFCPropertyGridProperty(_T("Blob Merge"), strTemp, _T(""));
	pGroupBlob_Merge->AddOption(_T("TRUE"));
	pGroupBlob_Merge->AddOption(_T("FALSE"));
	pGroupBlob->AddSubItem(pGroupBlob_Merge);

	CMFCPropertyGridProperty* pGroupBlob_MergeDist = new CMFCPropertyGridProperty(_T("Merge Distance"), params.nBlobMergeDist, _T(""));
	pGroupBlob->AddSubItem(pGroupBlob_MergeDist);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Etc.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	CMFCPropertyGridProperty* pGroupEtc = new CMFCPropertyGridProperty(_T("Etc."));
	m_propertyGridBlob.AddProperty(pGroupEtc);
}
