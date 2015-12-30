#include "pch.h"
#include <iostream>
#include "SDKDemoCommon.h"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   
using namespace Platform;
using namespace Platform::Collections;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Storage::Streams;
using namespace concurrency;
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;
using namespace Windows::UI::Xaml::Media::Imaging;
using namespace Windows::UI::Xaml::Interop;
using namespace Windows::ApplicationModel;
using namespace Windows::Storage::FileProperties;
using namespace Windows::Storage::Streams;
using namespace Windows::Graphics::Imaging;
using namespace Windows::UI::Popups;

using namespace foxitSDK;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//Class CMy_File
CMy_File::CMy_File()
{
}

CMy_File::~CMy_File()
{
}

void CMy_File::InitFileHandle(CMy_FileReadWrite* pFileHandle)
{
	clientData = pFileHandle;
	Release = g_PrivateRelease;
	GetSize = g_PrivateGetSize;
	ReadBlock = g_PrivateReadBlock;
	WriteBlock = g_PrivateWriteBlock;
	Flush = g_PrivateFlush;
	Truncate = g_PrivateTruncate;
}

void CMy_File::ReleaseFileHandle()
{
	if (clientData)
		delete clientData;
	clientData = NULL;
	Release = NULL;
	GetSize = NULL;
	ReadBlock = NULL;
	WriteBlock = NULL;
	Flush = NULL;
	Truncate = NULL;
}


void CMy_File::g_PrivateRelease(FS_LPVOID clientData)
{
	CMy_FileReadWrite* pFileData = (CMy_FileReadWrite*)clientData;

	if (pFileData)
	{
		delete pFileData;
	}
	pFileData = NULL;
}

FS_DWORD CMy_File::g_PrivateGetSize(FS_LPVOID clientData)
{
	CMy_FileReadWrite* pFileData = (CMy_FileReadWrite*)clientData;
	return pFileData->GetSize();
}

FS_RESULT CMy_File::g_PrivateReadBlock(FS_LPVOID clientData, FS_DWORD offset, FS_LPVOID buffer, FS_DWORD size)
{
	CMy_FileReadWrite* pFileData = (CMy_FileReadWrite*)clientData;

	size_t tSize = (size_t)size;

	if (!pFileData->ReadBlock(buffer, offset, tSize))
		return FSCRT_ERRCODE_ERROR;
	else
		return FSCRT_ERRCODE_SUCCESS;
	return FSCRT_ERRCODE_SUCCESS;
}

FS_RESULT CMy_File::g_PrivateWriteBlock(FS_LPVOID clientData, FS_DWORD offset, FS_LPCVOID buffer, FS_DWORD size)
{
	if (!clientData) return FSCRT_ERRCODE_FILE;

	//Change file pointer to offset
	CMy_FileReadWrite* pFileData = (CMy_FileReadWrite*)clientData;
	size_t tSize = (size_t)size;

	if (!pFileData->WriteBlock(buffer, offset, tSize))
		return FSCRT_ERRCODE_ERROR;
	else
		return FSCRT_ERRCODE_SUCCESS;
}

FS_RESULT CMy_File::g_PrivateFlush(FS_LPVOID clientData)
{
	return FSCRT_ERRCODE_SUCCESS;
}

FS_RESULT CMy_File::g_PrivateTruncate(FS_LPVOID clientData, FS_DWORD size)
{
	return FSCRT_ERRCODE_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Class CMy_FileReadWrite
CMy_FileReadWrite::CMy_FileReadWrite()
{
	m_pFile = nullptr;
}

CMy_FileReadWrite::~CMy_FileReadWrite()
{
	Release();
}

void CMy_FileReadWrite::SetSize(int nSize)
{
	m_iFileSize = nSize;
}

void CMy_FileReadWrite::CreateFileRead(Windows::Storage::StorageFile^ pFile, int nFileSize)
{
	m_pFile = pFile;
	m_iFileSize = nFileSize;
}

void CMy_FileReadWrite::Release()
{
	m_pFile = nullptr;
	if (m_Buffer && m_Buffer->Size > 0)
	{
		m_Buffer->Clear();
	}
}

int	CMy_FileReadWrite::GetSize()
{
	return m_iFileSize;
}

bool CMy_FileReadWrite::ReadBlock(void* buffer, int offset, size_t size)
{
	if (m_pFile == nullptr)
		return FALSE;

	bool _bIsReadOk = false;
	HANDLE processingHandle = CreateEventEx(
		NULL,
		NULL,
		false,
		EVENT_ALL_ACCESS
		);
	auto  OpenOp = m_pFile->OpenAsync(Windows::Storage::FileAccessMode::Read);
	OpenOp->Completed = ref new AsyncOperationCompletedHandler<IRandomAccessStream^>([buffer, offset, size, processingHandle, &_bIsReadOk](IAsyncOperation<IRandomAccessStream^>^ operation, Windows::Foundation::AsyncStatus status)
	{
		if (status == Windows::Foundation::AsyncStatus::Completed)
		{
			IRandomAccessStream^ readStream = operation->GetResults();
			readStream->Seek(offset);
			DataReader^ dataReader = ref new DataReader(readStream);
			auto LoadOp = dataReader->LoadAsync(size);
			LoadOp->Completed = ref new AsyncOperationCompletedHandler<unsigned int>([buffer, dataReader, processingHandle, &_bIsReadOk](IAsyncOperation<unsigned int>^ operation, Windows::Foundation::AsyncStatus status)
			{
				if (status == Windows::Foundation::AsyncStatus::Completed)
				{
					int numBytesLoaded = operation->GetResults();
					if (numBytesLoaded > 0)
					{
						Array<unsigned char, 1>^ fileContent = ref new Array<unsigned char, 1>(numBytesLoaded);
						dataReader->ReadBytes(fileContent);
						memcpy(buffer, fileContent->Data, numBytesLoaded);
						dataReader->DetachStream();
					}
					_bIsReadOk = true;
					SetEvent(processingHandle);
				}
				else
				{
					_bIsReadOk = false;
					OutputDebugString(L"LoadAsync status ERROR!!!!!\n");
					SetEvent(processingHandle);
				}
			});
		}
		else
		{
			_bIsReadOk = false;
			OutputDebugString(L"status ERROR!!!!!\n");
			SetEvent(processingHandle);
		}
	});
	WaitForSingleObjectEx(processingHandle, INFINITE, true);
	CloseHandle(processingHandle);
	return _bIsReadOk ? TRUE : FALSE;

}

bool CMy_FileReadWrite::WriteBlock(const void* buffer, int offset, size_t size)
{
	if (nullptr == m_Buffer)
	{
		m_Buffer = ref new Vector<Object^>();
	}

	Platform::Array<unsigned char>^ fileContent = ref new Platform::Array<unsigned char>((unsigned char*)buffer, size);
	m_Buffer->Append(fileContent);
	int size1 = m_Buffer->Size;
	return TRUE;
}

void CMy_FileReadWrite::ReleaseTempWriteBuffer()
{
	if (m_Buffer->Size > 0)
	{
		m_Buffer->Clear();
		m_Buffer = nullptr;
	}
}

Platform::Collections::Vector<Platform::Object^>^ CMy_FileReadWrite::GetFileBuffer()
{
	return m_Buffer;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Class FSDK_Document
FSDK_Document::FSDK_Document()
{
	m_pFileReader = NULL;
	m_pFileStream = NULL;

	FileHandle tempFile;
	tempFile.pointer = NULL;
	m_hFile = tempFile;

	DocHandle tempDoc;
	tempDoc.pointer = NULL;
	m_hDoc = tempDoc;

	PageHandle tempPage;
	tempPage.pointer = NULL;
	m_hPage = tempPage;
}

FSDK_Document::~FSDK_Document()
{
}

void	FSDK_Document::ReleaseResource()
{
	if (m_hPage.pointer)
	{
		FSPDF_Page_Clear((FSCRT_PAGE)(m_hPage.pointer));
		PageHandle tempPage;
		tempPage.pointer = NULL;
		m_hPage = tempPage;
	}
	if (m_hDoc.pointer)
	{
		FSPDF_Doc_Close((FSCRT_DOCUMENT)(m_hDoc.pointer));
		DocHandle tempDoc;
		tempDoc.pointer = NULL;
		m_hDoc = tempDoc;
	}
	if (m_hFile.pointer)
	{
		FSCRT_File_Release((FSCRT_FILE)(m_hFile.pointer));
		FileHandle tempFile;
		tempFile.pointer = NULL;
		m_hFile = tempFile;
	}
	if (m_pFileStream)
		delete m_pFileStream;
	m_pFileStream = NULL;

	m_pFileReader = NULL;
}


IAsyncOperation<IRandomAccessStreamWithContentType^>^ FSDK_Document::RenderPageAsync(PixelSource^ pxsrc, int iStartX, int iStartY, int iSizeX, int iSizeY, int iRotation, PageHandle page)
{
	return create_async([=]()->task < IRandomAccessStreamWithContentType^ > {

		bool ret = GetRenderBitmapData(pxsrc, iStartX, iStartY, iSizeX, iSizeY, iRotation, page);
		if (true != ret)
		{
			return create_task([]()->IRandomAccessStreamWithContentType^ {return nullptr; });
		}

		InMemoryRandomAccessStream^ _stream = ref new InMemoryRandomAccessStream();
		return task<BitmapEncoder^>(BitmapEncoder::CreateAsync(BitmapEncoder::BmpEncoderId, _stream)).then([=](BitmapEncoder^ encoder)->task < IRandomAccessStreamWithContentType^ > {
			DataReader^ reader = DataReader::FromBuffer(pxsrc->PixelBuffer);
			unsigned int bmp_length = pxsrc->PixelBuffer->Length;
			Array<unsigned char, 1>^ buffer = ref new Array<unsigned char, 1>(pxsrc->PixelBuffer->Length);
			if (!buffer)
			{
				return create_task([]()->IRandomAccessStreamWithContentType^ {return nullptr; });
			}
			reader->ReadBytes(buffer);

			encoder->SetPixelData(BitmapPixelFormat::Rgba8, BitmapAlphaMode::Straight, pxsrc->Width, pxsrc->Height, 96.0, 96.0, buffer);
			return task<void>(encoder->FlushAsync()).then([=]()->task < IRandomAccessStreamWithContentType^ > {
				RandomAccessStreamReference^ streamReference = RandomAccessStreamReference::CreateFromStream(_stream);
				return task<IRandomAccessStreamWithContentType^>(streamReference->OpenReadAsync()).then([=](IRandomAccessStreamWithContentType^ ad)->IRandomAccessStreamWithContentType^ {
					return ad;
				});
			});
		});
	});
}

IAsyncOperation<FS_RESULT>^  FSDK_Document::OpenDocumentAsync(Windows::Storage::StorageFile^ pdfFile, int32 iFileSize)
{
	return create_async([=]()->FS_RESULT {
		FS_RESULT iRet = FSCRT_ERRCODE_ERROR;
		if (nullptr == pdfFile)
			return iRet;

		m_pFileReader = new CMy_FileReadWrite();
		m_pFileReader->CreateFileRead(pdfFile, iFileSize);
		m_pFileStream = new CMy_File();
		m_pFileStream->InitFileHandle(m_pFileReader);
		FSCRT_FILE sdkFile = NULL;

		//Create a FSCRT_FILE object used for loading PDF document.
		iRet = FSCRT_File_Create(m_pFileStream, &sdkFile);

		if (iRet != FSCRT_ERRCODE_SUCCESS)
		{
			return iRet;
		}

		FSCRT_DOCUMENT sdkDoc;
		//Load PDF document
		iRet = FSPDF_Doc_StartLoad(sdkFile, NULL, &sdkDoc, NULL);
		if (iRet != FSCRT_ERRCODE_SUCCESS)
		{
			return iRet;
		}

		DocHandle CurDoc;
		FileHandle CurFile;
		CurDoc.pointer = (int64)sdkDoc;
		CurFile.pointer = (int64)sdkFile;

		m_hFile = CurFile;
		m_hDoc = CurDoc;

		/*std::string s;
		std::wstring ws = std::wstring(pdfFile->ToString()->Data());
		s.assign(ws.begin(), ws.end());
		FSCRT_BSTRC(filename, "D:/1.pdf");
		iRet = FSCRT_File_CreateFromFileName(&filename, FSCRT_FILEMODE_READONLY, &sdkFile);*/

		return iRet;
	});
}

FS_RESULT FSDK_Document::LoadPageSync(int32 iPageIndex)
{
	FS_RESULT iRet = FSCRT_ERRCODE_ERROR;
	FSCRT_PAGE pageGet;
	FSCRT_DOCUMENT sdkDoc = (FSCRT_DOCUMENT)m_hDoc.pointer;
	//Get page with specific page index.
	iRet = FSPDF_Doc_GetPage(sdkDoc, iPageIndex, &pageGet);
	if (iRet != FSCRT_ERRCODE_SUCCESS)
	{
		return iRet;
	}

	//Start to parse page
	FSCRT_PROGRESS progressParse = NULL;
	iRet = FSPDF_Page_StartParse(pageGet, FSPDF_PAGEPARSEFLAG_NORMAL, &progressParse);
	if (iRet != FSCRT_ERRCODE_FINISHED)
	{
		if (iRet != FSCRT_ERRCODE_SUCCESS)
		{
			return iRet;
		}
		//Continue parsing page.
		//If want to do progressive saving, please use the second parameter of FSCRT_Progress_Continue.
		//See FSCRT_Progress_Continue for more details.
		iRet = FSCRT_Progress_Continue(progressParse, NULL);
		FSCRT_Progress_Release(progressParse);
	}
	PageHandle CurPage;
	CurPage.pointer = (int64)pageGet;

	m_hPage = CurPage;
	return FSCRT_ERRCODE_SUCCESS;
}

FS_RESULT FSDK_Document::LoadTextPage(PageHandle page, TextPageHandle * textPage) {
	FSPDF_TEXTPAGE textPageTemp;
	return FSPDF_TextPage_Load((FSCRT_PAGE)(page.pointer), (FSPDF_TEXTPAGE *)(&(textPage->pointer)));
}

FS_RESULT FSDK_Document::LoadMatrix(PageHandle page, int iStartX, int iStartY, int iSizeX, int iSizeY, int iRotation, MatrixHandle * matrix) {
	FSCRT_MATRIX * mt;
	if (matrix->pointer == 0) {
		mt = new FSCRT_MATRIX;
		matrix->pointer = (int64)mt;
	}
	else {
		mt = (FSCRT_MATRIX *)(matrix->pointer);
	}
	return FSPDF_Page_GetMatrix((FSCRT_PAGE)(page.pointer), iStartX, iStartY, iSizeX, iSizeY, iRotation, mt);
}

FS_RESULT FSDK_Document::LoadAnnot(PageHandle page) {
	return FSPDF_Page_LoadAnnots((FSCRT_PAGE)(page.pointer));
}

FS_RESULT FSDK_Document::unloadAnnot(PageHandle page) {
	return FSPDF_Page_UnloadAnnots((FSCRT_PAGE)(page.pointer));
}

FS_RESULT FSDK_Document::getCharIndex(TextPageHandle textPage, MatrixHandle matrix, double x, double y, int * charIndex) {
	FSCRT_MATRIX * mt = (FSCRT_MATRIX *)(matrix.pointer);
	int xx = (x - mt->e)*mt->d - mt->c*(y - mt->f);
	xx /= (mt->a*mt->d - mt->c*mt->b);
	int yy = (y - mt->f)*mt->a - (x - mt->e)*mt->b;
	yy /= (mt->a*mt->d - mt->c*mt->b);

	//get character at the given posiotion
	return FSPDF_TextPage_GetCharIndexAtPos((FSPDF_TEXTPAGE)(textPage.pointer), xx, yy, 1, charIndex);
}

FS_RESULT FSDK_Document::getCharNum(TextPageHandle textPage, int * count) {
	return FSPDF_TextPage_CountChars((FSPDF_TEXTPAGE)(textPage.pointer), count);
}

FS_RESULT FSDK_Document::startSelectText(TextPageHandle textPage, int startIndex, int charNum) {
	return FSPDF_TextPage_SelectByRange((FSPDF_TEXTPAGE)(textPage.pointer), startIndex, charNum, &textselection);
}

FS_RESULT FSDK_Document::getRectNum(int * Num) {
	if (textselection == NULL) {
		return FSCRT_ERRCODE_ERROR;
	}
	return FSPDF_TextSelection_CountPieces(textselection, Num);
}

FS_RESULT FSDK_Document::getRect(int rectIndex, MatrixHandle matrix, int * x, int * y, int * z, int * w) {
	if (textselection == NULL || matrix.pointer == 0) {
		return FSCRT_ERRCODE_ERROR;
	}
	FSCRT_RECTF rect;
	if (FSPDF_TextSelection_GetPieceRect(textselection, rectIndex, &rect) != FSCRT_ERRCODE_SUCCESS) {
		return FSCRT_ERRCODE_ERROR;
	};
	FSCRT_MATRIX * mt = (FSCRT_MATRIX *)(matrix.pointer);
	*x = rect.left * mt->a + rect.bottom * mt->b + mt->e;
	*y = rect.left * mt->c + rect.bottom * mt->d + mt->f;
	*z = rect.right * mt->a + rect.top * mt->b + mt->e;
	*w = rect.right * mt->c + rect.top * mt->d + mt->f;
	return FSCRT_ERRCODE_SUCCESS;
}

Platform::String^ FSDK_Document::getChar(TextPageHandle textPage, int startIndex, int charNum, int * errCode) {
	FSCRT_BSTR textTemp;
	textTemp.str = NULL;
	FSPDF_TextPage_GetChars((FSPDF_TEXTPAGE)(textPage.pointer), startIndex, charNum, &textTemp);
	std::string std_string_word(textTemp.str);
	std::wstring std_wstring_word;
	std_wstring_word.assign(std_string_word.begin(), std_string_word.end());
	Platform::String^ textContent = ref new Platform::String(std_wstring_word.c_str());
	*errCode = FSCRT_ERRCODE_SUCCESS;
	return textContent;
}

FS_RESULT FSDK_Document::endSelectText() {
	if (textselection == NULL) {
		return FSCRT_ERRCODE_SUCCESS;
	}
	if (FSPDF_TextSelection_Release(textselection) != FSCRT_ERRCODE_SUCCESS) {
		return FSCRT_ERRCODE_ERROR;
	}
	else {
		textselection = NULL;
		return FSCRT_ERRCODE_SUCCESS;
	}
}

FS_RESULT FSDK_Document::addAnnot(PageHandle page, TextPageHandle textPage, MatrixHandle matrix, Platform::String^ word) {
	FS_RESULT ret;
	FS_BOOL isMatchTemp;
	FSPDF_TEXTSEARCH textSearchTemp = NULL;
	FSPDF_TEXTSELECTION textSelectionTemp = NULL;
	FSCRT_ANNOT annotTemp;
	FSCRT_BSTR wordTemp;
	std::string s;
	std::wstring ws = std::wstring(word->Data());
	s.assign(ws.begin(), ws.end());

	FSCRT_BStr_Init(&wordTemp);
	FSCRT_BStr_Set(&wordTemp, s.c_str(), s.length());
	/*
	if (strcmp(wordTemp.str, "Abstract") == 0) {
		FSPDF_Page_LoadAnnots((FSCRT_PAGE)(page.pointer));
		return 0;
	}
	*/
	ret = FSPDF_TextPage_StartSearch((FSPDF_TEXTPAGE)(textPage.pointer), &wordTemp, 0, 0, &textSearchTemp);
	FSPDF_TextSearch_FindNext(textSearchTemp, &isMatchTemp);
	//ret = FSPDF_Page_LoadAnnots((FSCRT_PAGE)(page.pointer));
	FSCRT_BSTR bsAnnotType;
	FSCRT_BStr_Init(&bsAnnotType);
	FSCRT_BStr_Set(&bsAnnotType, FSPDF_ANNOTTYPE_SQUIGGLY, 8);
	while (isMatchTemp) {
		ret = FSPDF_TextSearch_GetSelection(textSearchTemp, &textSelectionTemp);
		FSCRT_RECTF rect;
		int num;
		ret = FSPDF_TextSelection_CountPieces(textSelectionTemp, &num);
		for (int count = 0; count < num; count++) {
			FSCRT_RECTF rect = { 0, 0, 0, 0 };
			//Prepare the string object of the annotation filter.
			//Add an annotation to a specific index with specific filter.
			ret = FSPDF_Annot_Add((FSCRT_PAGE)(page.pointer), &rect, &bsAnnotType, &bsAnnotType, 1, &annotTemp);
			if (FSCRT_ERRCODE_SUCCESS != ret)
			{

			}
			ret = FSPDF_TextSelection_GetPieceRect(textSelectionTemp, count, &rect);
			if (count == 1) {
				return (int)(rect.left);
			}
			//Set the quadrilaterals points of annotation.
			FSCRT_QUADPOINTSF quadPoints = { rect.left, rect.top, rect.right, rect.top, rect.left, rect.bottom, rect.right, rect.bottom };
			//FSCRT_QUADPOINTSF quadPoints = { 0, 0, 500, 0, 0, 500, 500, 500 };
			FSPDF_Annot_SetQuadPoints(annotTemp, &quadPoints, 1);
			//Set the stroke color and opacity of annotation.
			FSPDF_Annot_SetHighlightingMode(annotTemp, FSPDF_ANNOT_HIGHLIGHTINGMODE_OUTLINE);
			FSPDF_Annot_SetColor(annotTemp, FALSE, 0x000000FF);
			FSPDF_Annot_SetOpacity(annotTemp, (FS_FLOAT)0.55);
		}

		FSPDF_TextSearch_FindNext(textSearchTemp, &isMatchTemp);
	}
	/*
	PixelSource^ test1 = ref new PixelSource();
	RenderPageAsync(test1, 0, 0, 500, 500, 0, page);
	FSPDF_Page_UnloadAnnots((FSCRT_PAGE)(page.pointer));
	*/
	/*
	if (strcmp(wordTemp.str, "Fast") != 0) {
		FSPDF_Page_UnloadAnnots((FSCRT_PAGE)(page.pointer));
	}
	*/
	FSPDF_TextSelection_Release(textSelectionTemp);
	FSPDF_TextSearch_Release(textSearchTemp);
	return FSCRT_ERRCODE_SUCCESS;
}
bool FSDK_Document::GetRenderBitmapData(PixelSource^ pxsrc, int iStartX, int iStartY, int iSizeX, int iSizeY, int iRotation, PageHandle page)
{
	FSCRT_BITMAP renderBmp = NULL;
	FSCRT_PAGE pdfPage = (FSCRT_PAGE)page.pointer;
	
	//Render page to SDK bitmap.
	FS_RESULT iRet = FSDK_PageToBitmap(pdfPage, pxsrc->Width, pxsrc->Height, iStartX, iStartY, iSizeX, iSizeY, iRotation, &renderBmp);
	if (FSCRT_ERRCODE_SUCCESS != iRet)
	{
		return false;
	}

	//Get data of SDK bitmap.
	iRet = FSDK_GetSDKBitmapData(renderBmp, pxsrc);
	FSCRT_Bitmap_Release(renderBmp);
	if (FSCRT_ERRCODE_SUCCESS != iRet)
	{
		return false;
	}
	else
	{
		return true;
	}
}

IObservableVector<Object^>^ FSDK_Document::SaveAsPDF()
{
	FSCRT_DOCUMENT pDoc = (FSCRT_DOCUMENT)m_hDoc.pointer;
	FSCRT_FILE fxFile = (FSCRT_FILE)m_hFile.pointer;
	if (!pDoc || !fxFile)
		return nullptr;

	Platform::Collections::Vector<Platform::Object^>^ filebytes = nullptr;

	//Start saving PDF file
	FSCRT_PROGRESS progress = NULL;
	FS_RESULT ret = FSPDF_Doc_StartSaveToFile(pDoc, fxFile, FSPDF_SAVEFLAG_INCREMENTAL, &progress);
	if (ret == FSCRT_ERRCODE_SUCCESS)
	{
		//Continue to finish saving
		//If want to do progressive saving, please use the second parameter of FSCRT_Progress_Continue.
		//See FSCRT_Progress_Continue for more details.
		ret = FSCRT_Progress_Continue(progress, NULL);
		//Release saving progress object
		FSCRT_Progress_Release(progress);
	}

	if (m_pFileReader)
	{
		filebytes = ref new Vector<Object^>();
		filebytes = m_pFileReader->GetFileBuffer();
	}

	return filebytes;
}

concurrency::task<IOutputStream^> FSDK_Document::WriteByteToFile(StorageFile^ file, IObservableVector<Object^>^ fileBytes)
{
	std::shared_ptr<DataWriter^> _spDataWriter = std::make_shared<DataWriter^>(nullptr);
	if (nullptr == file)
	{
		return create_task([]()->IOutputStream^ {return nullptr; });
	}
	auto OpenOp = file->OpenAsync(FileAccessMode::ReadWrite);
	return create_task(OpenOp).then([=](IRandomAccessStream^ writeStream)
	{
		writeStream->Seek(0);
		*_spDataWriter = ref new DataWriter(writeStream);
		auto size = fileBytes->Size;
		for (unsigned int i = 0; i < size; i++)
		{
			Platform::Array<unsigned char>^ fileContent = safe_cast<Platform::Array<unsigned char>^>(fileBytes->GetAt(i));
			(*_spDataWriter)->WriteBytes(fileContent);
		}
		return (*_spDataWriter)->StoreAsync();
	}).then([=](unsigned int bytesStored) {
		return (*_spDataWriter)->FlushAsync();
	}).then([=](bool flushSucceeded) {
		fileBytes->Clear();
		return (*_spDataWriter)->DetachStream();
	});
}

Windows::Foundation::IAsyncOperation<Boolean>^ FSDK_Document::SaveAsDocument(Windows::Storage::StorageFile^ file)
{
	return concurrency::create_async([=]()
	{
		if (nullptr == file)
		{
			return create_task([]()->Boolean {return false; });
		}
		return concurrency::create_task([=]()
		{
			Windows::Foundation::Collections::IObservableVector<Object^>^ buffer = SaveAsPDF();
			return buffer;
		})
			.then([=](Windows::Foundation::Collections::IObservableVector<Object^>^ fileBytes)
		{
			return WriteByteToFile(file, fileBytes);
		})
			.then([=](concurrency::task<IOutputStream^> t)
		{
			if (nullptr == t.get())
				return false;
			return true;
		});
	});
}
///////////////////////////////////////////////////////

/* Callback functions for FSCRT_MEMMGRHANDLER*/
//Extension to allocate memory buffer. Implementation to FSCRT_MEMMGRHANDLER::Alloc
static FS_LPVOID	FSDK_Alloc(FS_LPVOID clientData, FS_DWORD size)
{
	return malloc((size_t)size);
}

//Extension to reallocate memory buffer. Implementation to FSCRT_MEMMGRHANDLER::Realloc
static FS_LPVOID	FSDK_Realloc(FS_LPVOID clientData, FS_LPVOID ptr, FS_DWORD newSize)
{
	return realloc(ptr, (size_t)newSize);
}

//Extension to free memory buffer. Implementation to FSCRT_MEMMGRHANDLER::Free
static void			FSDK_Free(FS_LPVOID clientData, FS_LPVOID ptr)
{
	free(ptr);
}
/* END: Callback functions for FSCRT_MEMMGRHANDLER*/

static FSCRT_MEMMGRHANDLER	g_MemMgrHandler = { NULL, FSDK_Alloc, FSDK_Realloc, FSDK_Free };


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Inherited_PDFFunction::Inherited_PDFFunction()
{

}

FS_RESULT Inherited_PDFFunction::FSDK_Initialize()
{
	FS_RESULT ret = FSCRT_Library_CreateDefaultMgr();
	if (ret == FSCRT_ERRCODE_SUCCESS)
	{
		//Unlock library, otherwise no methods of SDK can be used
		FSCRT_BSTR license_id;
		FSCRT_BStr_Init(&license_id);
		license_id.str = (FS_LPSTR)"eZYIIG5pU+kg8I27xHQayAkk0lqZrRnJdcIP8ftI74z8OELWUiioiw==";
		license_id.len = strlen(license_id.str);
		FSCRT_BSTR unlockCode;
		unlockCode.str = (FS_LPSTR)"8f3o18ONtRkJBDdKtFJS8bag2amLgQDM5FPEGj3yDnut43GW9tBT6kcyfKOH77HHnqQrwVNtHI6DrcE9f2eLb0wGaKDSEvEZfBNORSR2zqne7VboQHCI+4IWFaN+FUUzO1U5Ms2/u6pj4w8opS7yMcAYKAY0CzkbZCjzVFNNItM+Yd6n9HSxQnBSIWXdH1fquVN9YhgYe1ub6c+JxsmvvWZYCfLITOvcnHOpnDTw1+mmBBQ9X4Vqb6zC+rlCh4M3A4XLagZwoYUnTlmMkU65g5Dtj9oDp3hkuQQxTIO3wNppYhxJ+jdQmpWhWo+qI0WZbqEBCFUpVWollnLbrI81AriVD+E2Joiyhdcs3tkjNoVKZmxCUerxUas1mqEn9b8M88sqmXujLMlpHm4mmJxtiNQ8O8tYgJqksyTVW4sAPlQvJT7YjZJo10xcuenIfnUACMMEVGKSHAAZXW+1nhq+Z4ZiRU8WY/GwLpQebOeb4HnrQosvEHHEH0DRg9DKZSPlYGH+5mLFzriZifGxPF4+j5dwJC0s8iccvB9FfqN89TVCAi1t39eJjPUQ9AoZOFrGvdtKp6FIm1otagbyl/RUjx5AqlDX0IyM8NPoaObOB0Bp1na4aU3tygaliDdtngOiV1njGxDmNBVYYp3iJLQQiDpTOvOCBYHvM5gA0KINWRqLgeKfzkwm2CMWd/hUpZgyZGVJf3TTEmYWUq+j8gylEWttQ9XQKgesIolkK//u1LyFgr1RejAIBEZylcC6eedi2XfIaQJeIMAHIxzhfICJJt+h81cNkoe6mXBN7gJSlWe6y+OE3rZ1tw3qHcpAVCBKxV686NBEhA3iToktYEzSGMUCPxU/F7nkQCxhyTJgeT9/auDYrGtUeiLqA4aVTuMkgxlOSoM+T+sEg7MctBqoCn3v1m2kPBdLuwFVxE30CwS9Csrn3vQ6xhc7PdQLdVRZ26Uy+PU4HAaimrd/yskBI3pGjBhyfmrHvL6RfEb/PtQxUO8xTyWqwr6v6n8EIbn+FNpOoTrAsLfqJrndcnHaDj8/IyB+nmcyL3fQh5Ni2DTDNQlUQ2Ic5yLU1rTRd3+rRhY6dvh4kWYm3BfSsFsv9LbM4LgPq9tXWaMZCIezL9H2Cs1Dx/i0m7o97l8b7yHtad+LNktEWDU9WAu5sgWu3Jgj/N//XAMC08JfJEEVlHtzvh1usU+6FIyH9h6g1Clsaxya2udViSNpiUH7wbfM7EprJwr2MoYi+a2cFhhl6+h/2SsAQ5q+Lae7Yg4U2SYS8V6oHpirqcQjcYTWY4LcK5de7GHYXTDDq6aAKrgB84ONn+i8hoA7pyXE+UC3mLA2dIYa9+LkqdbNRFvibJC3hcw1Ch0WmKwkH7npCqv/4nJHl96tkZ3hX+rJwHoCrOgMf/xajqi+nl1z/R18SbELp9XS7dSVDeM5JMBT4xbAPAfrSibA/FW5kgHGtnJdAs+nEqxHNSyEyMDKs3suKLUodmO8nwOsMrlLZwNSKQz9xR+xE6vLlNZg68Lu37ppQscCByv9pD3PQsLnqHrAwnuR+kt+X7xzYS8J3swvszLZwtgorODcljOL0WoVQ+jVHiSEbIXyIREl7pquP0/+XL9pHCdqVjsixwtedXcTxOj363Q9YDUKRvHfj1fZnL4J6VR6y1xqg5osvv6r+LAxGVk8aR2RayTMncRIuS6rgBAw/tdYNehhNPeNHBiDrA6BVkla27LzJUrR5KIm0Bn9yZWxFyQwehiKgCLQExRg0w==";
		unlockCode.len = strlen(unlockCode.str);
		ret = FSCRT_License_UnlockLibrary(&license_id, &unlockCode);
		if (FSCRT_ERRCODE_SUCCESS == ret)
		{
			//Load system font.
			FSCRT_Library_LoadSystemFonts();
			//Initialize PDF module in order to use methods about PDF.
			ret = FSCRT_PDFModule_Initialize();
			if (FSCRT_ERRCODE_SUCCESS != ret)
			{
				FSDK_Finalize();
			}
		}
	}

	return ret;
}

void Inherited_PDFFunction::FSDK_Finalize()
{
	//Finitialize PDF module.
	FSCRT_PDFModule_Finalize();

	//Destroy manager
	FSCRT_Library_DestroyMgr();

}

FS_RESULT	Inherited_PDFFunction::My_Page_Clear(PageHandle page)
{
	return FSPDF_Page_Clear((FSCRT_PAGE)(page.pointer));
}

FS_RESULT Inherited_PDFFunction::My_TextPage_Clear(TextPageHandle textPage)
{
	return FSPDF_TextPage_Release((FSPDF_TEXTPAGE)(textPage.pointer));
}

FS_RESULT Inherited_PDFFunction::My_Matrix_Clear(MatrixHandle matrix) {
	try {
		delete (FSCRT_MATRIX *)(matrix.pointer);
	}
	catch (...) {
		return FSCRT_ERRCODE_ERROR;
	}
	return FSCRT_ERRCODE_SUCCESS;
}

FS_RESULT	Inherited_PDFFunction::My_TextSearch_Clear(TextSearchHandle textSearch)
{
	return FSPDF_TextSearch_Release((FSPDF_TEXTSEARCH)(textSearch.pointer));
}
FS_RESULT Inherited_PDFFunction::My_Page_GetSize(PageHandle page, FS_FLOAT* width, FS_FLOAT* height)
{
	return FSPDF_Page_GetSize((FSCRT_PAGE)(page.pointer), width, height);
}

FS_INT32 Inherited_PDFFunction::My_Doc_CountPages(DocHandle doc)
{
	FS_INT32 count;
	FSPDF_Doc_CountPages((FSCRT_DOCUMENT)doc.pointer, &count);
	return count;
}

Platform::String^ Inherited_PDFFunction::GetWordFromLocation(PageHandle page, float x, float y, int iStartX, int iStartY, int iSizeX, int iSizeY, int iRotation)
{
	//load textpage
	FSPDF_TEXTPAGE textPage = NULL;
	FS_RESULT ret = FSPDF_TextPage_Load((FSCRT_PAGE)(page.pointer), &textPage); //Load text page
	if (ret != FSCRT_ERRCODE_SUCCESS)
	{
		Platform::String^ word = " 00";
		return word;
	}
	
	//getmatrix
	FSCRT_MATRIX mt;
	ret = FSPDF_Page_GetMatrix((FSCRT_PAGE)(page.pointer), iStartX, iStartY, iSizeX, iSizeY, iRotation, &mt);
	if (ret != FSCRT_ERRCODE_SUCCESS)
	{
		Platform::String^ word = " 01";
		return word;
	}

	//get xx and yy from x and y 
	//*(xx,yy) is coordinate in page             (x,y) is position in render image
	int xx = (x - mt.e)*mt.d - mt.c*(y - mt.f);
	xx /= (mt.a*mt.d - mt.c*mt.b);
	int yy = (y - mt.f)*mt.a - (x - mt.e)*mt.b;
	yy /= (mt.a*mt.d - mt.c*mt.b);

	//get character at the given posiotion
	FS_INT32 charIndex;
	ret = FSPDF_TextPage_GetCharIndexAtPos(textPage, xx, yy, 1, &charIndex);
	if (ret != FSCRT_ERRCODE_SUCCESS)
	{
		Platform::String^ word = " 02";
		return word;
	}

	FSCRT_BSTR tmp_char;
	tmp_char.str = NULL;
	int result;
	ret = FSPDF_TextPage_GetChars(textPage, charIndex, 1, &tmp_char);
	if (ret != FSCRT_ERRCODE_SUCCESS)
	{
		Platform::String^ word = " 03";
		return word;
	}
	if (tmp_char.len == 0 || IsCharacterValid(tmp_char.str[0]) == 0)
	{
		Platform::String^ word = " 04";
		return word;
	}

	//get pre-character and next character
	FS_INT32 preIndex = charIndex;
	FS_INT32 nextIndex = charIndex;
	do
	{
		preIndex--;
		ret = FSPDF_TextPage_GetChars(textPage, preIndex, 1, &tmp_char);
		if (ret != FSCRT_ERRCODE_SUCCESS)
		{
			Platform::String^ word = " 03";
			return word;
		}
		if (tmp_char.len == 0)
		{
			continue;
		}
		result = IsCharacterValid(tmp_char.str[0]);
	} while (result != 0);
	preIndex = preIndex + 1;
	do
	{
		nextIndex++;
		ret = FSPDF_TextPage_GetChars(textPage, nextIndex, 1, &tmp_char);
		if (ret != FSCRT_ERRCODE_SUCCESS)
		{
			Platform::String^ word = " 03";
			return word;
		}
		if (tmp_char.len == 0)
		{
			continue;
		}
		result = IsCharacterValid(tmp_char.str[0]);
	} while (result != 0);
	nextIndex--;
	
	
	//get all word
	ret = FSPDF_TextPage_GetChars(textPage, preIndex, nextIndex - preIndex + 1, &tmp_char);
	if (ret != FSCRT_ERRCODE_SUCCESS)
	{
		Platform::String^ word = " 03";
		return word;
	}

	//change uppcase to lowercase
	for (int i = 0; i < tmp_char.len; i++)
	{
		if (IsCharacterValid(tmp_char.str[i]) == 2)
		{
			tmp_char.str[i] += 'a' - 'A';
		}
		
	}
	std::string std_string_word(tmp_char.str);
	std::wstring std_wstring_word;
	std_wstring_word.assign(std_string_word.begin(), std_string_word.end());
	Platform::String^ word = ref new Platform::String(std_wstring_word.c_str());
	
	FSPDF_TextPage_Release(textPage);
	return word;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Search_PDFFunction::Search_PDFFunction(DocHandle doc, Platform::String^ searchText)
{
	FSPDF_Doc_CountPages((FSCRT_DOCUMENT)doc.pointer, &pageCount);
	Doc = (FSCRT_DOCUMENT)doc.pointer;

	std::string s;
	std::wstring ws = std::wstring(searchText->ToString()->Data());
	s.assign(ws.begin(), ws.end());

	//Set searchPattern
	FSCRT_BStr_Init(&searchPattern);
	FSCRT_BStr_Set(&searchPattern, (char*)s.c_str(), (FS_DWORD)s.length());
}

Search_PDFFunction::~Search_PDFFunction()
{
	FSCRT_BStr_Clear(&searchPattern);
	FSPDF_TextSearch_Release(textSearch);
	FSPDF_TextPage_Release(textPage);

}

void Search_PDFFunction::Finish_Search()
{
	FSCRT_BStr_Clear(&searchPattern);
	FSPDF_TextSearch_Release(textSearch);
	FSPDF_TextPage_Release(textPage);
}

FS_RESULT Search_PDFFunction::My_StartSearch(int iPageIndex, int* resultPageIndex)
{
	if (iPageIndex >= pageCount)
	{
		return FSCRT_ERRCODE_ERROR;
	}
	//search from iPageIndex find one page which contain searchtext
	this->HandlePage(iPageIndex);

	textSearch = NULL;
	FS_RESULT result = FSPDF_TextPage_StartSearch(textPage, &searchPattern, 0, 0, &textSearch);
	if (result != FSCRT_ERRCODE_SUCCESS)
	{
		return result;
	}
	FS_BOOL isMatch = false;
	result = FSPDF_TextSearch_FindNext(textSearch, &isMatch);
	if (result != FSCRT_ERRCODE_SUCCESS)
	{
		return result;
	}
	int jumpedPageNumber = 0;
	while (!isMatch)
	{
		jumpedPageNumber++;
		iPageIndex = (iPageIndex + 1) % pageCount;
		if (jumpedPageNumber == pageCount)
		{
			break;
			return 1;//mean not found the search text
		}
		FSPDF_TextPage_Release(textPage);
		result = this->HandlePage(iPageIndex);
		if (result != FSCRT_ERRCODE_SUCCESS)
		{
			return result;
		}
		FSPDF_TextSearch_Release(textSearch);
		textSearch = NULL;
		result = FSPDF_TextPage_StartSearch(textPage, &searchPattern, 0, 0, &textSearch);
		if (result != FSCRT_ERRCODE_SUCCESS)
		{
			return result;
		}
		result = FSPDF_TextSearch_FindNext(textSearch, &isMatch);
		if (result != FSCRT_ERRCODE_SUCCESS)
		{
			return result;
		}
	}
	currentPage = iPageIndex;

	//choose textselection in the found page
	FSPDF_TEXTSELECTION textSelection = NULL;
	result = FSPDF_TextSearch_GetSelection(textSearch, &textSelection);
	if (result != FSCRT_ERRCODE_SUCCESS)
	{
		return result;
	}
	FS_INT32 pieceCount;
	result = FSPDF_TextSelection_CountPieces(textSelection, &pieceCount);
	if (result != FSCRT_ERRCODE_SUCCESS)
	{
		return result;
	}
	for (int i = 0; i < pieceCount; i++)
	{
		FSCRT_RECTF selectionRect;
		FS_INT32 startCharIndex;
		FS_INT32 countCharIndex;
		FSCRT_RECTF selectedRect;
		result = FSPDF_TextSelection_GetPieceCharRange(textSelection, i, &startCharIndex, &countCharIndex);
		if (result != FSCRT_ERRCODE_SUCCESS)
		{
			return result;
		}
		result = FSPDF_TextSelection_GetPieceRect(textSelection, i, &selectedRect);
		if (result != FSCRT_ERRCODE_SUCCESS)
		{
			return result;
		}
		//output result or just use highlight function to render
	}
	FSPDF_TextSelection_Release(textSelection);
	resultPageIndex = &currentPage;
	return FSCRT_ERRCODE_SUCCESS;
}

FS_RESULT Search_PDFFunction::FindNext(int* resultPageIndex)
{
	//textsearch.findnext
	//if ismatch == fasle
	//find the next page and next next page;
	FS_BOOL isMatch = false;
	FS_RESULT result;
	result = FSPDF_TextSearch_FindNext(textSearch, &isMatch);
	if (result != FSCRT_ERRCODE_SUCCESS)
	{
		return result;
	}
	int jumpedPageNumber = 0;
	while (!isMatch)
	{
		//load and parse page and load textpage
		jumpedPageNumber++;
		currentPage = (currentPage + 1) % pageCount;
		FSPDF_TextPage_Release(textPage);
		result = this->HandlePage(currentPage);
		if (result != FSCRT_ERRCODE_SUCCESS)
		{
			return result;
		}
		//start search
		FSPDF_TextSearch_Release(textSearch);
		textSearch = NULL;
		result = FSPDF_TextPage_StartSearch(textPage, &searchPattern, 0, 0, &textSearch);
		if (result != FSCRT_ERRCODE_SUCCESS)
		{
			return result;
		}
		result = FSPDF_TextSearch_FindNext(textSearch, &isMatch);
		if (result != FSCRT_ERRCODE_SUCCESS)
		{
			return result;
		}
	}

	//choose textselection in the found page
	FSPDF_TEXTSELECTION textSelection = NULL;
	result = FSPDF_TextSearch_GetSelection(textSearch, &textSelection);
	if (result != FSCRT_ERRCODE_SUCCESS)
	{
		return result;
	}
	FS_INT32 pieceCount;
	result = FSPDF_TextSelection_CountPieces(textSelection, &pieceCount);
	if (result != FSCRT_ERRCODE_SUCCESS)
	{
		return result;
	}
	for (int i = 0; i < pieceCount; i++)
	{
		FSCRT_RECTF selectionRect;
		FS_INT32 startCharIndex;
		FS_INT32 countCharIndex;
		FSCRT_RECTF selectedRect;
		result = FSPDF_TextSelection_GetPieceCharRange(textSelection, i, &startCharIndex, &countCharIndex);
		if (result != FSCRT_ERRCODE_SUCCESS)
		{
			return result;
		}
		result = FSPDF_TextSelection_GetPieceRect(textSelection, i, &selectedRect);
		if (result != FSCRT_ERRCODE_SUCCESS)
		{
			return result;
		}
		//output result or just use highlight function to render
	}
	FSPDF_TextSelection_Release(textSelection);
	resultPageIndex = &currentPage;
	return FSCRT_ERRCODE_SUCCESS;
}

FS_RESULT Search_PDFFunction::FindPrevious(int* resultPageIndex)
{
	FS_BOOL isMatch = false;
	FS_RESULT result;
	result = FSPDF_TextSearch_FindPrev(textSearch, &isMatch);
	if (result != FSCRT_ERRCODE_SUCCESS)
	{
		return result;
	}
	int jumpedPageNumber = 0;
	while (!isMatch)
	{
		jumpedPageNumber++;
		currentPage = (currentPage - 1 + pageCount) % pageCount;
		FSPDF_TextPage_Release(textPage);
		result = this->HandlePage(currentPage);
		if (result != FSCRT_ERRCODE_SUCCESS)
		{
			return result;
		}
		FSPDF_TextSearch_Release(textSearch);
		textSearch = NULL;
		result = FSPDF_TextPage_StartSearch(textPage, &searchPattern, 0, 0, &textSearch);
		if (result != FSCRT_ERRCODE_SUCCESS)
		{
			return result;
		}
		result = FSPDF_TextSearch_FindPrev(textSearch, &isMatch);
		if (result != FSCRT_ERRCODE_SUCCESS)
		{
			return result;
		}
	}

	//choose textselection in the found page
	FSPDF_TEXTSELECTION textSelection = NULL;
	result = FSPDF_TextSearch_GetSelection(textSearch, &textSelection);
	if (result != FSCRT_ERRCODE_SUCCESS)
	{
		return result;
	}
	FS_INT32 pieceCount;
	result = FSPDF_TextSelection_CountPieces(textSelection, &pieceCount);
	if (result != FSCRT_ERRCODE_SUCCESS)
	{
		return result;
	}
	for (int i = 0; i < pieceCount; i++)
	{
		FSCRT_RECTF selectionRect;
		FS_INT32 startCharIndex;
		FS_INT32 countCharIndex;
		FSCRT_RECTF selectedRect;
		result = FSPDF_TextSelection_GetPieceCharRange(textSelection, i, &startCharIndex, &countCharIndex);
		if (result != FSCRT_ERRCODE_SUCCESS)
		{
			return result;
		}
		result = FSPDF_TextSelection_GetPieceRect(textSelection, i, &selectedRect);
		if (result != FSCRT_ERRCODE_SUCCESS)
		{
			return result;
		}
		//output result or just use highlight function to render
	}
	FSPDF_TextSelection_Release(textSelection);
	resultPageIndex = &currentPage;
	return FSCRT_ERRCODE_SUCCESS;
}

FS_RESULT Search_PDFFunction::FindAllResult()
{
	FS_RESULT result;
	for (int i = 0; i < pageCount; i++)
	{
		result = this->HandlePage(i);
		if (result != FSCRT_ERRCODE_SUCCESS)
		{
			return result;
		}
		textSearch = NULL;
		FS_RESULT result = FSPDF_TextPage_StartSearch(textPage, &searchPattern, 0, 0, &textSearch);
		if (result != FSCRT_ERRCODE_SUCCESS)
		{
			return result;
		}
		FS_BOOL isMatch = false;
		result = FSPDF_TextSearch_FindNext(textSearch, &isMatch);
		if (result != FSCRT_ERRCODE_SUCCESS)
		{
			return result;
		}
		while (isMatch)
		{
			FSPDF_TEXTSELECTION textSelection = NULL;
			result = FSPDF_TextSearch_GetSelection(textSearch, &textSelection);
			if (result != FSCRT_ERRCODE_SUCCESS)
			{
				return result;
			}
			FS_INT32 pieceCount = 0;
			result = FSPDF_TextSelection_CountPieces(textSelection, &pieceCount);
			if (result != FSCRT_ERRCODE_SUCCESS)
			{
				return result;
			}
			for (int i = 0; i < pieceCount; i++)
			{
				FSCRT_RECTF selectionRect;
				FS_INT32 startCharIndex = 0;
				FS_INT32 countCharIndex = 0;
				FSCRT_RECTF selectedRect;
				result = FSPDF_TextSelection_GetPieceCharRange(textSelection, i, &startCharIndex, &countCharIndex);
				if (result != FSCRT_ERRCODE_SUCCESS)
				{
					return result;
				}
				result = FSPDF_TextSelection_GetPieceRect(textSelection, i, &selectedRect);
				if (result != FSCRT_ERRCODE_SUCCESS)
				{
					return result;
				}
				//output result or just use highlight function to render
			}
			FSPDF_TextSelection_Release(textSelection);
			result = FSPDF_TextSearch_FindNext(textSearch, &isMatch);
			if (result != FSCRT_ERRCODE_SUCCESS)
			{
				return result;
			}
		}
		FSPDF_TextPage_Release(textPage);
		FSPDF_TextSearch_Release(textSearch);
	}
	return FSCRT_ERRCODE_SUCCESS;
}

FS_RESULT Search_PDFFunction::HandlePage(int iPageIndex)
{
	FSCRT_PAGE currentPage = NULL;
	FS_RESULT iRet = FSCRT_ERRCODE_ERROR;
	//Get page with specific page index.
	iRet = FSPDF_Doc_GetPage(Doc, iPageIndex, &currentPage);
	if (iRet != FSCRT_ERRCODE_SUCCESS)
	{
		return iRet;
	}

	//Start to parse page
	FSCRT_PROGRESS progressParse = NULL;
	iRet = FSPDF_Page_StartParse(currentPage, FSPDF_PAGEPARSEFLAG_NORMAL, &progressParse);
	if (iRet != FSCRT_ERRCODE_FINISHED)
	{
		if (iRet != FSCRT_ERRCODE_SUCCESS)
		{
			return iRet;
		}
		iRet = FSCRT_Progress_Continue(progressParse, NULL);
		FSCRT_Progress_Release(progressParse);
	}
	//load text page
	textPage = NULL;
	iRet = FSPDF_TextPage_Load(currentPage, &textPage);

	if (iRet != FSCRT_ERRCODE_SUCCESS)
	{
		return iRet;
	}
	return FSCRT_ERRCODE_SUCCESS;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


FS_RESULT Inherited_PDFFunction::hiliteText(PageHandle page, float startX, float startY, float endX, float endY, int iStartX, int iStartY, int iSizeX, int iSizeY, int iRotation) {
	int num;
	if (true) {
		
		FSPDF_TEXTPAGE textPage = NULL;
		FS_RESULT ret = FSPDF_TextPage_Load((FSCRT_PAGE)(page.pointer), &textPage); //Load text page
		if (ret != FSCRT_ERRCODE_SUCCESS)
		{
			return FSCRT_ERRCODE_ERROR;
		}

		//getmatrix
		FSCRT_MATRIX mt;
		ret = FSPDF_Page_GetMatrix((FSCRT_PAGE)(page.pointer), iStartX, iStartY, iSizeX, iSizeY, iRotation, &mt);
		if (ret != FSCRT_ERRCODE_SUCCESS)
		{
			return FSCRT_ERRCODE_ERROR;
		}

		//get xx and yy from x and y 
		//*(xx,yy) is coordinate in page             (x,y) is position in render image
		int sxx = (startX - mt.e)*mt.d - mt.c*(startY - mt.f);
		sxx /= (mt.a*mt.d - mt.c*mt.b);
		int syy = (startY - mt.f)*mt.a - (startX - mt.e)*mt.b;
		syy /= (mt.a*mt.d - mt.c*mt.b);

		int exx = (endX - mt.e)*mt.d - mt.c*(endY - mt.f);
		exx /= (mt.a*mt.d - mt.c*mt.b);
		int eyy = (endY - mt.f)*mt.a - (endX - mt.e)*mt.b;
		eyy /= (mt.a*mt.d - mt.c*mt.b);

		//get character at the given posiotion
		FS_INT32 startIndex, endIndex;
		ret = FSPDF_TextPage_GetCharIndexAtPos(textPage, sxx, syy, 1, &startIndex);
		ret = FSPDF_TextPage_GetCharIndexAtPos(textPage, exx, eyy, 1, &endIndex);
		startIndex = 100;
		endIndex = 200;
		if (startIndex == selectStartIndex && endIndex == selectEndIndex) {
			return FSCRT_ERRCODE_ERROR;
		}
		selectStartIndex = startIndex;
		selectEndIndex = endIndex;
		if (startIndex == endIndex){
			return FSCRT_ERRCODE_ERROR;
		}
		else if (startIndex > endIndex) {
			int temp = startIndex;
			startIndex = endIndex;
			endIndex = temp;
		}
		FSPDF_TEXTSELECTION m_pTextSelection;
		ret = FSPDF_TextPage_SelectByRange(textPage, startIndex, endIndex - startIndex + 1, &m_pTextSelection);
		FSCRT_RECTF rect;
		ret = FSPDF_TextSelection_CountPieces(m_pTextSelection, &num);
		ret = FSPDF_Page_LoadAnnots((FSCRT_PAGE)(page.pointer));
		for (int count = 0; count < num; count++) {
			FSCRT_RECTF rect = { 0, 0, 500, 500 };
			//Prepare the string object of the annotation filter.
			FSCRT_BSTR bsAnnotType;
			FSCRT_BStr_Init(&bsAnnotType);
			FSCRT_BStr_Set(&bsAnnotType, FSPDF_ANNOTTYPE_HIGHLIGHT, 9);
			//Add an annotation to a specific index with specific filter.
			ret = FSPDF_Annot_Add((FSCRT_PAGE)(page.pointer), &rect, &bsAnnotType, &bsAnnotType, 1, &selectAnnot);
			if (FSCRT_ERRCODE_SUCCESS != ret)
			{

			}
			ret = FSPDF_TextSelection_GetPieceRect(m_pTextSelection, count, &rect);
			//Set the quadrilaterals points of annotation.
			FSCRT_QUADPOINTSF quadPoints = { rect.left, rect.bottom, rect.right, rect.bottom, rect.left, rect.top, rect.right, rect.top };
			//FSCRT_QUADPOINTSF quadPoints = { 0, 0, 500, 0, 0, 500, 500, 500 };
			FSPDF_Annot_SetQuadPoints(selectAnnot, &quadPoints, 1);
			//Set the stroke color and opacity of annotation.
			FSPDF_Annot_SetHighlightingMode(selectAnnot, FSPDF_ANNOT_HIGHLIGHTINGMODE_OUTLINE);
			FSPDF_Annot_SetColor(selectAnnot, FALSE, 0x000000FF);
			FSPDF_Annot_SetOpacity(selectAnnot, (FS_FLOAT)0.55);
		}
	}
	//return FSCRT_ERRCODE_SUCCESS;
	return num;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

FS_RESULT foxitSDK::FSDK_PageToBitmap(FSCRT_PAGE page, int bmpWidth, int bmpHeight, int iStartX, int iStartY, int iSizeX, int iSizeY, int iRotation, FSCRT_BITMAP *renderBmp)
{
	FS_RESULT ret = FSCRT_ERRCODE_ERROR;
	//Get a bitmap handler to hold bitmap data from rendering progress.
	ret = FSCRT_Bitmap_Create((FS_INT32)bmpWidth, (FS_INT32)bmpHeight, FSCRT_BITMAPFORMAT_32BPP_RGBx, NULL, 0, renderBmp);
	if (ret != FSCRT_ERRCODE_SUCCESS)
	{
		return ret;
	}

	//Set rect area and fill the color of bitmap.
	FSCRT_RECT rect = { 0, 0, (FS_INT32)bmpWidth, (FS_INT32)bmpHeight };
	ret = FSCRT_Bitmap_FillRect(*renderBmp, FSCRT_ARGB_Encode(0xff, 0xff, 0xff, 0xff), &rect);
	if (ret != FSCRT_ERRCODE_SUCCESS)
	{
		FSCRT_Bitmap_Release(*renderBmp);
		return ret;
	}

	//Get the page's matrix.
	FSCRT_MATRIX mt;
	ret = FSPDF_Page_GetMatrix(page, iStartX, iStartY, iSizeX, iSizeY, iRotation, &mt);
	if (ret != FSCRT_ERRCODE_SUCCESS)
	{
		FSCRT_Bitmap_Release(*renderBmp);
		return ret;
	}

	//Create a renderer based on a given bitmap, and page will be rendered to this bitmap.
	FSCRT_RENDERER renderer;
	ret = FSCRT_Renderer_CreateOnBitmap(*renderBmp, &renderer);
	if (ret != FSCRT_ERRCODE_SUCCESS)
	{
		FSCRT_Bitmap_Release(*renderBmp);
		return ret;
	}

	//Create a render context used for rendering page.
	FSPDF_RENDERCONTEXT rendercontext;
	ret = FSPDF_RenderContext_Create(&rendercontext);
	if (ret != FSCRT_ERRCODE_SUCCESS)
	{
		FSCRT_Renderer_Release(renderer);
		FSCRT_Bitmap_Release(*renderBmp);
		return ret;
	}

	ret = FSPDF_RenderContext_SetFlags(rendercontext, FSPDF_RENDERCONTEXTFLAG_ANNOT);
	if (ret != FSCRT_ERRCODE_SUCCESS)
	{
		//Release render
		FSCRT_Renderer_Release(renderer);
		FSPDF_RenderContext_Release(rendercontext);
		FSCRT_Bitmap_Release(*renderBmp);
		return ret;
	}

	//Set the matrix of the given render context.
	ret = FSPDF_RenderContext_SetMatrix(rendercontext, &mt);
	if (ret != FSCRT_ERRCODE_SUCCESS)
	{
		FSCRT_Renderer_Release(renderer);
		FSPDF_RenderContext_Release(rendercontext);
		FSCRT_Bitmap_Release(*renderBmp);
		return ret;
	}

	//Start to render page.
	FSCRT_PROGRESS renderProgress = NULL;
	ret = FSPDF_RenderContext_StartPage(rendercontext, renderer, page, FSPDF_PAGERENDERFLAG_NORMAL, &renderProgress);
	if (ret != FSCRT_ERRCODE_SUCCESS)
	{
		FSCRT_Renderer_Release(renderer);
		FSPDF_RenderContext_Release(rendercontext);
		FSCRT_Bitmap_Release(*renderBmp);
		return ret;
	}

	//Continue render progress.
	//If want to do progressive rendering, please use the second parameter of FSCRT_Progress_Continue.
	//See FSCRT_Progress_Continue for more details.
	ret = FSCRT_Progress_Continue(renderProgress, NULL);

	FSCRT_Progress_Release(renderProgress);
	FSCRT_Renderer_Release(renderer);
	FSPDF_RenderContext_Release(rendercontext);
	if (FSCRT_ERRCODE_FINISHED == ret)
		ret = FSCRT_ERRCODE_SUCCESS;
	else
		FSCRT_Bitmap_Release(*renderBmp);

	return ret;
}

FS_RESULT foxitSDK::FSDK_GetSDKBitmapData(FSCRT_BITMAP bmp, PixelSource^ dib)
{
	void *lpBmpBuf = 0;
	FS_RESULT ret = FSCRT_Bitmap_GetLineBuffer(bmp, 0, &lpBmpBuf);
	if (ret != FSCRT_ERRCODE_SUCCESS)
	{
		return ret;
	}
	unsigned int width = 0, height = 0;
	ret = FSCRT_Bitmap_GetSize(bmp, (FS_INT32 *)(&width), (FS_INT32 *)(&height));
	if (ret != FSCRT_ERRCODE_SUCCESS)
	{
		return ret;
	}
	unsigned int stride = 0;
	ret = FSCRT_Bitmap_GetLineStride(bmp, (FS_INT32 *)(&stride));
	if (ret != FSCRT_ERRCODE_SUCCESS)
	{
		return ret;
	}
	unsigned int size = stride * height;
	FSCRT_BITMAPINFO bitmapInfo;
	FS_DWORD bmp_infosize;
	FSCRT_Bitmap_GetBitmapInfo(bmp, NULL, &bmp_infosize);
	FSCRT_Bitmap_GetBitmapInfo(bmp, &bitmapInfo, &bmp_infosize);
	Array<unsigned char, 1>^ buffer = ref new Array<unsigned char, 1>(size);
	memcpy(buffer->Data, lpBmpBuf, size);
	DataWriter ^writer = ref new DataWriter();
	writer->WriteBytes(buffer);
	dib->Format = PixelFormat::BGRx;
	dib->PixelBuffer = writer->DetachBuffer();
	if (!dib->PixelBuffer)
	{
		ret = FSCRT_ERRCODE_ERROR;
	}

	return ret;
}


/*std::shared_ptr<FSCRT_BSTR> foxitSDK::ConvertToFoxitString(Platform::String^ str)
{
	//shared_ptr is a smart pointer class. The first param in construct is the pointer, the second param is a dtor.
	std::shared_ptr<FSCRT_BSTR> _FSstr(new FSCRT_BSTR(), [](FSCRT_BSTR* param)
	{
		if (param)
			FSCRT_BStr_Clear(param);
	});

	FSCRT_BStr_Init(&*_FSstr);
	const wchar_t* _strptr = str->Data();
	int size = WideCharToMultiByte(CP_UTF8, 0, _strptr, -1, NULL, 0, NULL, NULL);
	char *temp = new char[size + 1];
	WideCharToMultiByte(CP_UTF8, 0, _strptr, -1, temp, size, NULL, NULL);
	FSCRT_BStr_Set(&*_FSstr, (FS_LPSTR)temp, strlen(temp));
	delete temp;

	return _FSstr;
}*/

void foxitSDK::ShowErrorLog(Platform::String^ errorContent, UICommandInvokedHandler^ returnInvokedHandler,
	bool bSDKError/* = false*/, FS_RESULT errorcode/* = FSCRT_ERRCODE_ERROR*/)
{
	if (bSDKError)
		errorContent += "\r\nError status value: " + errorcode.ToString() + "."
		+ "\r\nPlease check the value in macro definitions FSCRT_ERRCODE_XXX.";

	MessageDialog^ msgDialog = ref new MessageDialog(errorContent);
	// Add "OK" command.
	UICommand^ defaultCommand = ref new UICommand("OK", nullptr);
	msgDialog->Commands->Append(defaultCommand);
	// Set the command that will be invoked by default
	msgDialog->DefaultCommandIndex = 0;
	if (nullptr != returnInvokedHandler)
	{
		//Add "Back To MainPage" command and set its callback.
		UICommand^ returnCommand = ref new UICommand("Back To MainPage", returnInvokedHandler);
		msgDialog->Commands->Append(returnCommand);
		// Set the command to be invoked when escape is pressed
		msgDialog->CancelCommandIndex = 1;
	}

	msgDialog->ShowAsync();
}

FS_RESULT foxitSDK::IsCharacterValid(char character)
{
	int ch = (int)character;
	if (ch == '\n' || ch == '\r' )
	{
		return 0;
	}
	else if (ch == '-')
	{
		return 0;
	}
	else if (ch > 'A' && ch < 'Z')
	{
		return 2;
	}
	else if (ch > 'a' && ch < 'z')
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
