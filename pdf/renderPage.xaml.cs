using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;
using foxitSDK;
using Windows.UI.Xaml.Media.Imaging;
using Windows.UI.ViewManagement;
// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=234238

/*
PDF SDK DEFINE:

 @brief	No rotation. 
#define	FSCRT_PAGEROTATION_0		0
 @brief	Rotate 90 degrees in clockwise direction.
#define	FSCRT_PAGEROTATION_90		1
 @brief	Rotate 180 degrees in clockwise direction. 
#define	FSCRT_PAGEROTATION_180		2
 @brief	Rotate 270 degrees in clockwise direction. 
#define	FSCRT_PAGEROTATION_270		3

#define FSCRT_ERRCODE_SUCCESS					0
#define FSCRT_ERRCODE_ERROR						-1
typedef int						FS_RESULT;
*/


namespace pdf
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    /// 
	public struct pageInfo
    {
        public PageHandle hPage;
        public TextPageHandle hTextPage;
        public MatrixHandle hMatrix;
        public TextSearchHandle hTextSearch;
        public byte[] bitmap;
        public int bmpWidth;
        public int bmpHeight;
    }

    public struct wordInfo
    {
        public string word;
        public bool[] annotFlag;
    }
	
    public sealed partial class renderPage : Page
    {
        

        int rSelectPageStartIndex;
        int rSelectPageEndIndex;
        int rSelectCharStartIndex;
        int rSelectCharEndIndex;

        byte[] selectBmpTemp;

        bool ctrlDown;

        List<wordInfo> wordInfoList;
        bool[] annotLoadFlagList;
		
        public renderPage()
        {
            this.InitializeComponent();

            /////////////////////////////////

            m_bReleaseLibrary = false;
            m_SDKDocument = null;

            m_PDFFunction = new Inherited_PDFFunction();

            //Initialize, otherwise no method of SDK can be used.
            int iRet = m_PDFFunction.FSDK_Initialize();
            if (0 != iRet)
            {
                return;
            }
            m_bReleaseLibrary = true;

            m_SDKDocument = new FSDK_Document();

            m_PDFDoc.pointer = 0;
            m_PDFPage.pointer = 0;

            //m_iCurPageIndex = 0;
            m_iPageCount = 0;
            m_fPageWidth = 0.0f;
            m_fPageHeight = 0.0f;

            m_iStartX = 0;
            m_iStartY = 0;
            m_iRenderAreaSizeX = 0;
            m_iRenderAreaSizeY = 0;
            m_iRotation = 0;

            m_dbScaleDelta = 0.25f;
            m_dbScaleFator = 1.0f;
            m_dbCommonFitWidthScale = 1.0f;
            m_dbCommonFitHeightScale = 1.0f;
            //m_dbRotateFitWidthScale = 1.0f;
            //m_dbRotateFitHeightScale = 1.0f;

            m_bFitWidth = false;
            m_bFitHeight = false;

            m_mousestate = false;

            firstVisibleIndex = 0;
            nextInvisibleIndex = 0;
            visiblePage = new Dictionary<int, pageInfo>();
            shadowHeight = 10;
			//totalPageNum = 0;

            selectPageStartIndex = -1;
            selectPageEndIndex = -1;
            selectCharStartIndex = -1;
            selectCharEndIndex = -1;

            rSelectPageStartIndex = -1;
            rSelectPageEndIndex = -1;
            rSelectCharStartIndex = -1;
            rSelectCharEndIndex = -1;

            selectBmpTemp = new byte[0];

            Windows.UI.Core.CoreWindow.GetForCurrentThread().KeyDown += RenderPage_KeyDown;
            Windows.UI.Core.CoreWindow.GetForCurrentThread().KeyUp += RenderPage_KeyUp;
            
            wordInfoList = new List<wordInfo>();
        }
        ////////////////////////////////////////////////////////////////////copy and paste
        
		private void RenderPage_KeyDown(Windows.UI.Core.CoreWindow sender, Windows.UI.Core.KeyEventArgs args)
        {
            if(args.VirtualKey == Windows.System.VirtualKey.Control)
            {
                ctrlDown = true;
            }
            else if(args.VirtualKey == Windows.System.VirtualKey.C && ctrlDown == true)
            {
                Windows.ApplicationModel.DataTransfer.DataPackage dp = new Windows.ApplicationModel.DataTransfer.DataPackage();
                dp.RequestedOperation = Windows.ApplicationModel.DataTransfer.DataPackageOperation.Copy;
                string selectText = getSelectText();
                dp.SetText(selectText);
                Windows.ApplicationModel.DataTransfer.Clipboard.SetContent(dp);
            }
        }
        private void RenderPage_KeyUp(Windows.UI.Core.CoreWindow sender, Windows.UI.Core.KeyEventArgs args)
        {
            if(args.VirtualKey == Windows.System.VirtualKey.Control)
            {
                ctrlDown = false;
            }
        }

        ///////////////////////////////////////////////////////////////

        void ReleaseResources()
        {
            if (m_SDKDocument != null)
                m_SDKDocument.ReleaseResource();
            m_PDFFunction.FSDK_Finalize();
            m_bReleaseLibrary = false;
        }

        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            m_pPDFFile = e.Parameter as Windows.Storage.StorageFile;
            if (m_pPDFFile != null)
            {
                //please import wordlist from database to wordinfolist
                OpenPDFDocument(m_pPDFFile);
            }
        }

        //////////////////////////////////////////////////////////////initialize renderpage grid and reset the size of grid       
        private void ud_visibilityChanged(DependencyObject sender, DependencyPropertyChangedEventArgs e)
        {
            int a = 1;
        }

        void initPdfGrid(int pageNum)
        {
            ColumnDefinition cd = new ColumnDefinition();
            ud_pdfContainer.ColumnDefinitions.Add(cd);
            DependencyProperty dp = DependencyProperty.RegisterAttached("visibilityProperty", typeof(Visibility), typeof(Image), new Windows.UI.Xaml.PropertyMetadata(Visibility.Collapsed, ud_visibilityChanged));
            for (int count = 0;count < pageNum;count++)
            {
                RowDefinition rd = new RowDefinition();
                ud_pdfContainer.RowDefinitions.Add(rd);

                Image img = new Image();
                img.Name = "" + count;
                img.Stretch = Stretch.Uniform;
                img.Width = m_iRenderAreaSizeX;

                img.Height = m_iRenderAreaSizeY + shadowHeight;
                img.PointerPressed += GetBeginLocation;
                img.PointerMoved += GetMoveLocation;
                img.PointerReleased += GetEndLocation;

                Binding bd= new Binding();
                bd.Source = img;
                bd.Path = new PropertyPath("Visibility");
                img.SetBinding(dp, bd);
                
                ud_pdfContainer.Children.Add(img);
                Grid.SetColumn(img, 0);
                Grid.SetRow(img, count);

            }
            string[] testStr = new string[0];
            wordInfo wInfo;
            for (int count = 0; count < testStr.Length; count++)
            {
                bool[] testData = new bool[pageNum];
                for (int count1 = 0; count1 < testData.Length; count1++)
                {
                    testData[count1] = false;
                }
                wInfo.word = testStr[count];
                wInfo.annotFlag = testData;
                wordInfoList.Add(wInfo);
            }
        }

        void ResetPdfGrid(int pageNum)
        {
            for(int count = 0; count < pageNum; count++)
            {
                Image img = (Image)(ud_pdfContainer.Children.ElementAt(count));
                img.Height = m_iRenderAreaSizeY + shadowHeight;
                img.Width = m_iRenderAreaSizeX;
            }
        }

        //////////////////////////////////////////////////////////////// open pdf and load pdf and calculate pdf size and unload pdf      
        private async void OpenPDFDocument(Windows.Storage.StorageFile file)
        {
            if (file != null)
            {
                Windows.Storage.FileProperties.BasicProperties properties = await file.GetBasicPropertiesAsync();
                m_SDKDocument = new FSDK_Document();
                //Load PDF document
                int result = await m_SDKDocument.OpenDocumentAsync(file, (int)properties.Size);
                if(result != 0)
                {
                    return;
                }
                m_PDFDoc.pointer = m_SDKDocument.m_hDoc.pointer;

                //Load first two PDF page
                result = LoadPage(0);
                if(result != 0)
                {
                    return;
                }
                result = LoadPage(1);
                if (result != 0)
                {
                    return;
                }
                nextInvisibleIndex = 2;
                m_iPageCount = m_PDFFunction.My_Doc_CountPages(m_PDFDoc);
                
                //get page size and calculate suitable render size
                GetPageInfo();
                m_dbScaleFator = (m_dbCommonFitWidthScale < m_dbCommonFitHeightScale) ? m_dbCommonFitWidthScale : m_dbCommonFitHeightScale;
                CalcRenderSize();

                //Show PDF page to device.
                initPdfGrid(m_iPageCount);
                ShowPage();

                //initial preview page 
                preview_visiblePage = new Dictionary<int, PageHandle>();
                Preview_InitGrid();
            }
            return;

        }

        private int LoadPage(int iPageIndex)
        {// To load and parse page.
            
            int result = -1;
            if (m_PDFDoc.pointer == 0)
            {
                return result;
            }
            if (!visiblePage.ContainsKey(iPageIndex))
            {
                if(preview_visiblePage != null && preview_visiblePage.ContainsKey(iPageIndex))
                {
                    pageInfo pageptr = new pageInfo();
                    pageptr.hPage = preview_visiblePage[iPageIndex];
                    visiblePage.Add(iPageIndex, pageptr);
                    result = 0;
                }
                else
                {
                    result = m_SDKDocument.LoadPageSync(iPageIndex);
                    if (0 == result)
                    {
                        m_PDFPage.pointer = m_SDKDocument.m_hPage.pointer;
                        pageInfo pageptr = new pageInfo();
                		pageptr.hPage.pointer = m_SDKDocument.m_hPage.pointer;
						pageptr.hTextPage.pointer = 0;
                		pageptr.hMatrix.pointer = 0;
                        visiblePage.Add(iPageIndex, pageptr);
                        //m_iCurPageIndex = iPageIndex;
                        //GetPageInfo();
                    }
                }         
            }
            else
            {
                result = 0;
            }
            return result;
        }

        private void unLoadPage(int iPageIndex)
        {
            if (visiblePage.ContainsKey(iPageIndex))
            {
				if (visiblePage[iPageIndex].hTextSearch.pointer != 0)
                {
                    m_PDFFunction.My_TextSearch_Clear(visiblePage[iPageIndex].hTextSearch);
                }
                if (visiblePage[iPageIndex].hMatrix.pointer != 0)
                {
                    m_PDFFunction.My_Matrix_Clear(visiblePage[iPageIndex].hMatrix);
                }
                if (visiblePage[iPageIndex].hTextPage.pointer != 0)
                {
                    m_PDFFunction.My_TextPage_Clear(visiblePage[iPageIndex].hTextPage);
                }
				if (visiblePage[iPageIndex].hPage.pointer != 0)
                {
                	if(preview_visiblePage != null && !preview_visiblePage.ContainsKey(iPageIndex))
                	{
                    	m_PDFFunction.My_Page_Clear(visiblePage[iPageIndex].hPage);
                	}
				}
                visiblePage.Remove(iPageIndex);
                Image img = (Image)(ud_pdfContainer.Children.ElementAt(iPageIndex));
                img.Source = null;
            }
            return;         
        }

        public void GetPageInfo()
        {// To get page size and prepare some necessary data.
            if (m_PDFPage.pointer == 0)
            {
               return;
            }
            int result = m_PDFFunction.My_Page_GetSize(m_PDFPage, out m_fPageWidth,out m_fPageHeight);///////////////////
            if (result == 0)
            {
                double scrollViewerW = scroll_pdf.ActualWidth;
                double scrollViewerH = scroll_pdf.ActualHeight;

                m_dbCommonFitWidthScale = scrollViewerW / m_fPageWidth;
                m_dbCommonFitHeightScale = scrollViewerH / m_fPageHeight;

                //m_dbRotateFitWidthScale = scrollViewerW / m_fPageHeight;
                //m_dbRotateFitHeightScale = scrollViewerH / m_fPageWidth;
            }
        }

        public void CalcRenderSize()
        {// To calculate render size.
         /*if (1 == m_iRotation || 3 == m_iRotation)
         {
             if (m_bFitWidth)
                 m_dbScaleFator = m_dbRotateFitWidthScale;
             if (m_bFitHeight)
                 m_dbScaleFator = m_dbRotateFitHeightScale;

             m_iRenderAreaSizeX = (int)(m_fPageHeight * m_dbScaleFator);
             m_iRenderAreaSizeY = (int)(m_fPageWidth * m_dbScaleFator);
         }
         else
         {*/
            if (m_bFitWidth)
                m_dbScaleFator = m_dbCommonFitWidthScale;
            if (m_bFitHeight)
                m_dbScaleFator = m_dbCommonFitHeightScale;

            m_iRenderAreaSizeX = (int)(m_fPageWidth * m_dbScaleFator);
            m_iRenderAreaSizeY = (int)(m_fPageHeight * m_dbScaleFator);
            //}

        }

        public async void ShowPage(int startIndex = -1, int endIndex = 0)
        {//To render PDF page, finally to the image control.	
         //Calculate render size.

            if (startIndex == -1)
            {
                startIndex = firstVisibleIndex;
                endIndex = nextInvisibleIndex;
            }
            for (int count = startIndex; count < endIndex; count++)
            {
				m_SDKDocument.LoadAnnot(visiblePage[count].hPage);
                //show word in wordlist on page
                foreach(wordInfo wInfo in wordInfoList)
                {
                    if(wInfo.annotFlag[count] == false)
                    {
                        addAnnot(count, wInfo.word);
                        wInfo.annotFlag[count] = true;
                    }
			    }
                
                Image img = (Image)(ud_pdfContainer.Children.ElementAt(count));

                PixelSource bitmap = new PixelSource();
                bitmap.Width = m_iRenderAreaSizeX;
                bitmap.Height = m_iRenderAreaSizeY;
                if (!visiblePage.ContainsKey(count))
                {
                    continue;
                }
                Windows.Storage.Streams.IRandomAccessStreamWithContentType stream = await m_SDKDocument.RenderPageAsync(bitmap, m_iStartX, m_iStartY, m_iRenderAreaSizeX, m_iRenderAreaSizeY, m_iRotation, visiblePage[count].hPage);
                if (stream == null)
                {
                    return;
                }
				
				pageInfo pginfo = visiblePage[count];
				
				pginfo.bitmap = bitmap.PixelBuffer.ToArray();
				pginfo.bmpHeight = bitmap.Height;
                pginfo.bmpWidth = bitmap.Width;

                if (count >= rSelectPageStartIndex && count <= rSelectPageEndIndex)
                {
                    m_SDKDocument.LoadTextPage(visiblePage[count].hPage, out pginfo.hTextPage);
                    visiblePage[count] = pginfo;
                    drawSelection(count);
                }
                else
                {
                    visiblePage[count] = pginfo;
                    Windows.UI.Xaml.Media.Imaging.BitmapImage bmpImage = new Windows.UI.Xaml.Media.Imaging.BitmapImage();
                    bmpImage.SetSource(stream);
                    img.Source = bmpImage;
            	}
				m_SDKDocument.unloadAnnot(visiblePage[count].hPage);
            }
        }

        //////////////////////////////////////////////////////////////////////////////////////////////// selection
        public string getSelectText()
        {
            string strRet = "";
            int selectCharStartIndexTemp = -1;
            int selectCharEndIndexTemp = -1;
            int flag;
            int errCode;
            pageInfo pginfo;
            pginfo.hTextPage.pointer = 0;
            for (int count = rSelectPageStartIndex; count <= rSelectPageEndIndex; count++)
            {
                if (!visiblePage.ContainsKey(count))
                {
                    m_SDKDocument.LoadPageSync(count);
                    m_PDFPage.pointer = m_SDKDocument.m_hPage.pointer;
                    pginfo.hPage.pointer = m_SDKDocument.m_hPage.pointer;
                    m_SDKDocument.LoadTextPage(pginfo.hPage, out pginfo.hTextPage);
                }
                else
                {
                    pginfo = visiblePage[count];
                }
                if (count == rSelectPageStartIndex)
                {
                    selectCharStartIndexTemp = rSelectCharStartIndex;
                }
                else
                {
                    selectCharStartIndexTemp = 0;
                }

                if (count < rSelectPageEndIndex)
                {
                    flag = m_SDKDocument.getCharNum(pginfo.hTextPage, out selectCharEndIndexTemp);
                    if (flag != 0)
                    {
                        return "";
                    }
                }
                else if (count == rSelectPageEndIndex)
                {
                    selectCharEndIndexTemp = rSelectCharEndIndex;
                }

                if (selectCharEndIndexTemp < selectCharStartIndexTemp)
                {
                    continue;
                }
                strRet += m_SDKDocument.getChar(pginfo.hTextPage, selectCharStartIndexTemp, selectCharEndIndexTemp - selectCharStartIndexTemp + 1, out errCode);
            }
            return strRet;
        }

        public async void drawSelection(int iPageIndex)
        {
            int selectCharStartIndexTemp = -1;
            int selectCharEndIndexTemp = -1;
            int flag;
            pageInfo pginfo;
            Image img;
            Windows.Storage.Streams.InMemoryRandomAccessStream ms;
            Windows.Graphics.Imaging.BitmapEncoder encoder;
            Windows.UI.Xaml.Media.Imaging.BitmapImage newImg;
            if (visiblePage.ContainsKey(iPageIndex) && visiblePage[iPageIndex].hTextPage.pointer != 0)
            {
                pginfo = visiblePage[iPageIndex];
                Windows.Storage.Streams.InMemoryRandomAccessStream stream = new Windows.Storage.Streams.InMemoryRandomAccessStream();
                if (selectBmpTemp.Length != pginfo.bitmap.Length)
                {
                    selectBmpTemp = new byte[pginfo.bitmap.Length];
                }
                Buffer.BlockCopy(pginfo.bitmap, 0, selectBmpTemp, 0, pginfo.bitmap.Length);
                if (iPageIndex < rSelectPageStartIndex || iPageIndex > rSelectPageEndIndex)
                {
                    img = (Image)(ud_pdfContainer.Children.ElementAt(iPageIndex));
                    ms = new Windows.Storage.Streams.InMemoryRandomAccessStream();
                    encoder = await Windows.Graphics.Imaging.BitmapEncoder.CreateAsync(Windows.Graphics.Imaging.BitmapEncoder.BmpEncoderId, ms);
                    encoder.SetPixelData(Windows.Graphics.Imaging.BitmapPixelFormat.Rgba8, Windows.Graphics.Imaging.BitmapAlphaMode.Straight, (uint)m_iRenderAreaSizeX, (uint)m_iRenderAreaSizeY, 96.0, 96.0, selectBmpTemp);
                    await encoder.FlushAsync();
                    newImg = new Windows.UI.Xaml.Media.Imaging.BitmapImage();
                    newImg.SetSource(ms);
                    img.Source = newImg;
                    return;
                }

                if(iPageIndex == rSelectPageStartIndex)
                {
                    selectCharStartIndexTemp = rSelectCharStartIndex;
                }
                else
                {
                    selectCharStartIndexTemp = 0;
                }

                if(iPageIndex < rSelectPageEndIndex)
                {
                    flag = m_SDKDocument.getCharNum(visiblePage[iPageIndex].hTextPage, out selectCharEndIndexTemp);
                    if(flag != 0)
                    {
                        return;
                    }
                }
                else if(iPageIndex == rSelectPageEndIndex)
                {
                    selectCharEndIndexTemp = rSelectCharEndIndex;
                }

                img = (Image)(ud_pdfContainer.Children.ElementAt(iPageIndex));
                
                m_SDKDocument.LoadMatrix(pginfo.hPage, m_iStartX, m_iStartY, m_iRenderAreaSizeX, m_iRenderAreaSizeY, m_iRotation, out pginfo.hMatrix);
                m_SDKDocument.startSelectText(pginfo.hTextPage, selectCharStartIndexTemp, selectCharEndIndexTemp - selectCharStartIndexTemp + 1);
                int rectNum;
                int x, y, z, w;
                m_SDKDocument.getRectNum(out rectNum);
                for(int count = 0;count < rectNum;count++)
                {
                    m_SDKDocument.getRect(count, pginfo.hMatrix, out x, out y, out z, out w);
                    
                    for(int count1 = w;count1 <= y;count1 ++)
                    {
                        for(int count2 = 4 * x;count2 <= 4 * z;count2 += 1)
                        {
                            selectBmpTemp[count1 * pginfo.bmpWidth * 4 + count2] /= 2;
                        }
                    }
}
                m_SDKDocument.endSelectText();
                //
                ms = new Windows.Storage.Streams.InMemoryRandomAccessStream();
                encoder = await Windows.Graphics.Imaging.BitmapEncoder.CreateAsync(Windows.Graphics.Imaging.BitmapEncoder.BmpEncoderId, ms);
                encoder.SetPixelData(Windows.Graphics.Imaging.BitmapPixelFormat.Rgba8, Windows.Graphics.Imaging.BitmapAlphaMode.Straight, (uint)m_iRenderAreaSizeX, (uint)m_iRenderAreaSizeY, 96.0, 96.0, selectBmpTemp);
                await encoder.FlushAsync();
                newImg = new Windows.UI.Xaml.Media.Imaging.BitmapImage();
                newImg.SetSource(ms);
                img.Source = newImg;
            }
        }

        public void addAnnot(int iPageIndex, string word)
        {
            pageInfo pgInfo;
            if (!visiblePage.ContainsKey(iPageIndex))
            {
                return;
            }
            pgInfo = visiblePage[iPageIndex];
            if (pgInfo.hTextPage.pointer == 0)
            {
                m_SDKDocument.LoadTextPage(pgInfo.hPage, out pgInfo.hTextPage);
            }
            if(pgInfo.hMatrix.pointer == 0)
            {
                m_SDKDocument.LoadMatrix(pgInfo.hPage, 0, 0, m_iRenderAreaSizeX, m_iRenderAreaSizeY, 0, out pgInfo.hMatrix);
            }
            int num = m_SDKDocument.addAnnot(pgInfo.hPage, pgInfo.hTextPage, pgInfo.hMatrix, word);
        }

        ///////////////////////////////////////////////////////////////////main body pdf renderpage scroll changed function   
        private void ud_scrollChanged(object sender, ScrollViewerViewChangedEventArgs e)
        {
            updatePageView();
        }

        private void ud_scrollSizeChanged(object sender, SizeChangedEventArgs e)
        {
            updatePageView();
        }

        private void updatePageView()
        {
            if (m_iRenderAreaSizeY <= 0 || m_iRenderAreaSizeY <= 0)
            {
                return;
            }
            int firstVisibleIndexTemp = Math.Max((int)Math.Floor(scroll_pdf.VerticalOffset / (m_iRenderAreaSizeY + shadowHeight)) - 1, 0);
            int nextInvisibleIndexTemp = Math.Min((int)Math.Floor((scroll_pdf.VerticalOffset + scroll_pdf.ActualHeight) / (m_iRenderAreaSizeY + shadowHeight)) + 2, m_PDFFunction.My_Doc_CountPages(m_PDFDoc));
            int pageNumTemp1;

            if (firstVisibleIndexTemp != firstVisibleIndex || nextInvisibleIndexTemp != nextInvisibleIndex)
            {
                if (firstVisibleIndexTemp <= firstVisibleIndex)
                {
                    pageNumTemp1 = Math.Min(nextInvisibleIndexTemp, firstVisibleIndex);
                    if (firstVisibleIndexTemp < pageNumTemp1)
                    {
                        for (int count = firstVisibleIndexTemp; count < pageNumTemp1; count++)
                        {
                            LoadPage(count);
                        }
                        ShowPage(firstVisibleIndexTemp, pageNumTemp1);
                    }
                    if (nextInvisibleIndex < nextInvisibleIndexTemp)
                    {
                        for (int count = nextInvisibleIndex; count < nextInvisibleIndexTemp; count++)
                        {
                            LoadPage(count);
                        }
                        ShowPage(nextInvisibleIndex, nextInvisibleIndexTemp);
                    }
                    pageNumTemp1 = Math.Max(nextInvisibleIndexTemp, firstVisibleIndex);
                    for (int count = pageNumTemp1; count < nextInvisibleIndex; count++)
                    {
                        unLoadPage(count);
                    }
                }
                else
                {
                    pageNumTemp1 = Math.Min(nextInvisibleIndex, firstVisibleIndexTemp);
                    for (int count = firstVisibleIndex; count < pageNumTemp1; count++)
                    {
                        unLoadPage(count);
                    }
                    for (int count = nextInvisibleIndexTemp; count < nextInvisibleIndex; count++)
                    {
                        unLoadPage(count);
                    }
                    pageNumTemp1 = Math.Max(nextInvisibleIndex, firstVisibleIndexTemp);
                    if (pageNumTemp1 < nextInvisibleIndexTemp)
                    {
                        for (int count = pageNumTemp1; count < nextInvisibleIndexTemp; count++)
                        {
                            LoadPage(count);
                        }
                        ShowPage(pageNumTemp1, nextInvisibleIndexTemp);
                    }
                }
                firstVisibleIndex = firstVisibleIndexTemp;
                nextInvisibleIndex = nextInvisibleIndexTemp;
            }
        }

        //////////////////////////////////////////////////////////////////////////////////////////////

        private Windows.Storage.StorageFile	        m_pPDFFile;
		private bool                                m_bReleaseLibrary;             // A flag used to indicate if SDK library has been initialized successfully and needs to be released.
        private FSDK_Document                       m_SDKDocument;
        private DocHandle                           m_PDFDoc;
        private PageHandle                          m_PDFPage;
        
        private int                                 m_iCurPageIndex;               //Index of current loaded page.
        private int                                 m_iPageCount;
        private float                               m_fPageWidth;             // Page width of current page.
        private float                               m_fPageHeight;            // Page Height of current page.
        
        //used for rendering page;
        private int m_iStartX;
        private int m_iStartY;
        private int m_iRenderAreaSizeX;
        private int m_iRenderAreaSizeY;
        private int m_iRotation;

        //Used for zooming.
        private double m_dbScaleDelta;
        private double m_dbScaleFator;
        private double m_dbCommonFitWidthScale;
        private double m_dbCommonFitHeightScale;
        private double m_dbRotateFitWidthScale;
        private double m_dbRotateFitHeightScale;
        private bool m_bFitWidth;
        private bool m_bFitHeight;

        //press event
        private Point m_BeginLocation;
        private Point m_EndLocation;
        private Inherited_PDFFunction m_PDFFunction;
        private bool m_mousestate;

        int firstVisibleIndex;
        int nextInvisibleIndex;
        Dictionary<int, pageInfo> visiblePage;

        ///////////////////////////render image size
        int shadowHeight;


       ////////////////////////////////selection
        int selectPageStartIndex;
        int selectPageEndIndex;
        int selectCharStartIndex;
        int selectCharEndIndex;

        ///////////////////////////////////////////////////////////////zoom in and zoom out event

        /*private void Click_BTN_NextPage(object sender, RoutedEventArgs e)
        {//Button click event:to turn to the next page
            if (m_PDFDoc.pointer == 0 || m_PDFPage.pointer == 0)
            {
                //showerrorlog
                return;
            }
            int iPageCount;
            iPageCount = m_PDFFunction.My_Doc_CountPages(m_PDFDoc);
            int iPageIndex = m_iCurPageIndex;
            if(iPageIndex < iPageCount - 1)
            {
                iPageIndex++;
                int result = LoadPage(iPageIndex);
                if(result != 0)
                {
                    return;
                }
                ShowPage();
            }
        }

        private void Click_BTN_PrePage(object sender, RoutedEventArgs e)
        {// Button click event: to turn to the previous page
            if (m_PDFDoc.pointer == 0 || m_PDFPage.pointer == 0)
            {
                //ShowErrorLog("Error: No PDF document or page has been loaded successfully.", ref new UICommandInvokedHandler(this, &demo_view::renderPage::ReturnCommandInvokedHandler));
                return;
            }
            int iPageCount;
            iPageCount = m_PDFFunction.My_Doc_CountPages(m_PDFDoc);
            int iPageIndex = m_iCurPageIndex;

            if (iPageIndex > 0)
            {
                iPageIndex--;
                int result = LoadPage(iPageIndex);
                if (result != 0)
                {
                    return;
                }
                ShowPage();
            }
        }
        private void Click_BTN_FitHeight(object sender, RoutedEventArgs e)
        {

        }

        private void Click_BTN_FitWidth(object sender, RoutedEventArgs e)
        {

        }

        private void Click_BTN_ActualSize(object sender, RoutedEventArgs e)
        {

        }

        private void Click_BTN_RotateRight(object sender, RoutedEventArgs e)
        {

        }

        private void Click_BTN_RotateLeft(object sender, RoutedEventArgs e)
        {

        }
        */
        private void Click_BTN_ZoomIn(object sender, RoutedEventArgs e)
        {// Button click event: to zoom in the page.
            if (m_PDFDoc.pointer == 0 || m_PDFPage.pointer == 0)
            {
                //ShowErrorLog("Error: No PDF document or page has been loaded successfully.", ref new UICommandInvokedHandler(this, &demo_view::renderPage::ReturnCommandInvokedHandler));
                return;
            }

            if (m_dbScaleFator + m_dbScaleDelta > 2.5)
                return;

            m_bFitHeight = false;
            m_bFitWidth = false;
            m_dbScaleFator += m_dbScaleDelta;

            CalcRenderSize();
            ResetPdfGrid(m_iPageCount);
            ShowPage();

        }

        private void Click_BTN_ZoomOut(object sender, RoutedEventArgs e)
        {// Button click event: to zoom out the page.
            if (m_PDFDoc.pointer == 0 || m_PDFPage.pointer == 0)
            {
                //ShowErrorLog("Error: No PDF document or page has been loaded successfully.", ref new UICommandInvokedHandler(this, &demo_view::renderPage::ReturnCommandInvokedHandler));
                return;
            }

            if (m_dbScaleFator - m_dbScaleDelta < 0.9)
                return;

            m_bFitHeight = false;
            m_bFitWidth = false;
            m_dbScaleFator -= m_dbScaleDelta;

            CalcRenderSize();
            ResetPdfGrid(m_iPageCount);
            ShowPage();

        }

        /////////////////////////////////////////////////////////////////press event function        

        private void GetBeginLocation(object sender, PointerRoutedEventArgs e)
        {
            int flag;
            if (m_mousestate == false)
            {
                if(selectPageStartIndex != -1)
                {
                    int indexTemp1 = Math.Max(rSelectPageStartIndex, firstVisibleIndex);
                    int indexTemp2 = Math.Min(rSelectPageEndIndex, nextInvisibleIndex - 1);
                    selectPageStartIndex = -1;
                    selectPageEndIndex = -1;
                    selectCharStartIndex = -1;
                    selectCharEndIndex = -1;

                    rSelectPageStartIndex = -1;
                    rSelectPageEndIndex = -1;
                    rSelectCharStartIndex = -1;
                    rSelectCharEndIndex = -1;
                    for (int count = indexTemp1; count <= indexTemp2; count++)
                    {
                        drawSelection(count);
                    }
                }
                m_mousestate = true;
                Windows.UI.Input.PointerPoint location = e.GetCurrentPoint((Image)sender);
                m_BeginLocation = location.Position;
                selectPageStartIndex = int.Parse(((Image)sender).Name);
                if (!visiblePage.ContainsKey(selectPageStartIndex))
                {
                    selectPageStartIndex = -1;
                    selectCharStartIndex = -1;
                    return;
                }
                pageInfo pginfo = visiblePage[selectPageStartIndex];
                if (visiblePage[selectPageStartIndex].hTextPage.pointer == 0)
                {
                    flag = m_SDKDocument.LoadTextPage(visiblePage[selectPageStartIndex].hPage, out pginfo.hTextPage);
                    if (flag != 0)
                    {
                        selectPageStartIndex = -1;
                        selectCharStartIndex = -1;
                        return;
                    }
                }
                flag = m_SDKDocument.LoadMatrix(pginfo.hPage, m_iStartX, m_iStartY, m_iRenderAreaSizeX, m_iRenderAreaSizeY, m_iRotation, out pginfo.hMatrix);
                if (flag != 0)
                {
                    selectPageStartIndex = -1;
                    selectCharStartIndex = -1;
                    return;
                }
                flag = m_SDKDocument.getCharIndex(pginfo.hTextPage, pginfo.hMatrix, m_BeginLocation.X, m_BeginLocation.Y, out selectCharStartIndex);
                if (flag != 0)
                {
                    selectPageStartIndex = -1;
                    selectCharStartIndex = -1;
                    return;
                }
                visiblePage[selectPageStartIndex] = pginfo;
            }
        }

        private void GetEndLocation(object sender, PointerRoutedEventArgs e)
        {
            Image clicked = (Image)sender;
            int currentPage = Int32.Parse(clicked.Name);

            if (m_mousestate == true)
            {
                Windows.UI.Input.PointerPoint location = e.GetCurrentPoint((Image)sender);
                m_EndLocation = location.Position;
                m_mousestate = false;
                if (m_BeginLocation.X == m_EndLocation.X && m_BeginLocation.Y == m_EndLocation.Y)
                {
                    //get word
                    System.String word = null ;
                    if(!visiblePage.ContainsKey(currentPage))
                    {
                        return;
                    }
                    word = m_PDFFunction.GetWordFromLocation(visiblePage[currentPage].hPage,(float)m_BeginLocation.X, (float)m_BeginLocation.Y,m_iStartX, m_iStartY, m_iRenderAreaSizeX, m_iRenderAreaSizeY, m_iRotation);
                    //popup definition of the word
                    //add the word wordinfolist
                    //word = m_PDFFunction.GetWordFromLocation(visiblePage[currentPage].hPage, (float)m_BeginLocation.X, (float)m_BeginLocation.Y, m_iStartX, m_iStartY, m_iRenderAreaSizeX, m_iRenderAreaSizeY, m_iRotation);
                    wordInfo wInfo;
                    bool[] testData = new bool[m_iPageCount];
                    for (int count1 = 0; count1 < testData.Length; count1++)
                    {
                        testData[count1] = false;
                    }
                    wInfo.word = word;
                    wInfo.annotFlag = testData;
                    wordInfoList.Add(wInfo);
                }
                else
                {
                    ;
                }
            }
            
         }

        private void GetMoveLocation(object sender, PointerRoutedEventArgs e)
        {
            int flag;
            int selectPageEndIndexTemp;
            int selectCharEndIndexTemp;
            int indexTemp;
            if (m_mousestate == true)
            {
                Windows.UI.Input.PointerPoint location = e.GetCurrentPoint((Image)sender);
                m_EndLocation = location.Position;
                //runtime highlight
                selectPageEndIndexTemp = int.Parse(((Image)sender).Name);
                if (!visiblePage.ContainsKey(selectPageEndIndexTemp))
                {
                    selectPageEndIndexTemp = -1;
                    selectCharEndIndexTemp = -1;
                    return;
                }
                pageInfo pginfo = visiblePage[selectPageEndIndexTemp];
                if (visiblePage[selectPageEndIndexTemp].hTextPage.pointer == 0)
                {
                    flag = m_SDKDocument.LoadTextPage(visiblePage[selectPageEndIndexTemp].hPage, out pginfo.hTextPage);
                    if (flag != 0)
                    {
                        selectPageEndIndexTemp = -1;
                        selectCharEndIndexTemp = -1;
                        return;
                    }
                }
                flag = m_SDKDocument.LoadMatrix(pginfo.hPage, m_iStartX, m_iStartY, m_iRenderAreaSizeX, m_iRenderAreaSizeY, m_iRotation, out pginfo.hMatrix);
                if (flag != 0)
                {
                    selectPageEndIndexTemp = -1;
                    selectCharEndIndexTemp = -1;
                    return;
                }
                flag = m_SDKDocument.getCharIndex(pginfo.hTextPage, pginfo.hMatrix, m_EndLocation.X, m_EndLocation.Y, out selectCharEndIndexTemp);
                if (flag != 0)
                {
                    selectPageEndIndexTemp = -1;
                    selectCharEndIndexTemp = -1;
                    return;
                }
                if (selectPageStartIndex == -1)
                {
                    selectPageStartIndex = selectPageEndIndexTemp;
                    selectCharStartIndex = selectCharEndIndexTemp;
                    return;
                }
                visiblePage[selectPageEndIndexTemp] = pginfo;
                if (selectPageEndIndexTemp != selectPageEndIndex || selectCharEndIndexTemp != selectCharEndIndex)
                {
                    //TBC
                    int pageIndexTemp;
                    int pageNumTemp;
                    pageIndexTemp = Math.Min(selectPageEndIndex, selectPageEndIndexTemp);
                    pageNumTemp = Math.Abs(selectPageEndIndex - selectPageEndIndexTemp);
                    selectPageEndIndex = selectPageEndIndexTemp;
                    selectCharEndIndex = selectCharEndIndexTemp;
                    if (selectPageStartIndex < selectPageEndIndex)
                    {
                        rSelectPageStartIndex = selectPageStartIndex;
                        rSelectCharStartIndex = selectCharStartIndex;
                        rSelectPageEndIndex = selectPageEndIndex;
                        rSelectCharEndIndex = selectCharEndIndex;
                    }
                    else if (selectPageStartIndex == selectPageEndIndex)
                    {
                        rSelectPageStartIndex = selectPageStartIndex;
                        rSelectPageEndIndex = selectPageEndIndex;
                        if (selectCharStartIndex < selectCharEndIndex)
                        {
                            rSelectCharStartIndex = selectCharStartIndex;
                            rSelectCharEndIndex = selectCharEndIndex;
                        }
                        else
                        {
                            rSelectCharStartIndex = selectCharEndIndex;
                            rSelectCharEndIndex = selectCharStartIndex;
                        }
                    }
                    else
                    {
                        rSelectPageStartIndex = selectPageEndIndex;
                        rSelectCharStartIndex = selectCharEndIndex;
                        rSelectPageEndIndex = selectPageStartIndex;
                        rSelectCharEndIndex = selectCharStartIndex;
                    }
                    /*
                    if (rSelectPageStartIndex != rSelectPageEndIndex)
                    {
                        int a = 1;
                    }
                    */
                    for (int count = 0; count <= pageNumTemp; count++)
                    {
                        drawSelection(pageIndexTemp + count);
                    }
                }
            }
        }

        /////////////////////////////////////////////////////
        private void Jump_Page(int iPageIndex)
        {
            iPageIndex++;

            preview_popup.IsOpen = false;

            UpdateLayout();

            /*for (int i = firstVisibleIndex; i < nextInvisibleIndex; i++)
            {
                unLoadPage(i);
            }

            firstVisibleIndex = Math.Max(iPageIndex , 0);
            nextInvisibleIndex = Math.Min(iPageIndex + (int)(scroll_pdf.ActualHeight / (m_iRenderAreaSizeY + shadowHeight)) + 2, m_iPageCount);

            for (int i = firstVisibleIndex; i < nextInvisibleIndex; i++)
            {
                int result = LoadPage(i);
                if (result != 0)
                    return;
            }
            ShowPage();
            */

            scroll_pdf.ScrollToVerticalOffset((iPageIndex - 1) * (m_iRenderAreaSizeY + shadowHeight));

            preview_popup.IsOpen = true;
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////preview page in popup

        private int preview_firstVisibleIndex;
        private int preview_nextInvisibleIndex;

        Dictionary<int, PageHandle> preview_visiblePage;
        private int preview_scaleDown = 6;
        private double preview_unitHeight;//height of unit item of grid
        
        void Preview_InitGrid()
        {

            ColumnDefinition cd = new ColumnDefinition();
            preview_grid.ColumnDefinitions.Add(cd);
            DependencyProperty dp = DependencyProperty.RegisterAttached("visibilityProperty", typeof(Visibility), typeof(Image), new Windows.UI.Xaml.PropertyMetadata(Visibility.Collapsed, ud_visibilityChanged));
            for (int count = 0; count < m_iPageCount; count++)
            {
                RowDefinition rd = new RowDefinition();
                preview_grid.RowDefinitions.Add(rd);

                Image page_Image = new Image();
                page_Image.Width = m_fPageWidth / preview_scaleDown;
                page_Image.Height = m_fPageHeight / preview_scaleDown;
                page_Image.Stretch = Stretch.Fill;
                
                Binding bd = new Binding();
                bd.Source = page_Image;
                bd.Path = new PropertyPath("Visibility");
                page_Image.SetBinding(dp, bd);

                TextBlock page_Label = new TextBlock();
                page_Label.Height = 18;
                page_Label.Text = (1 + count).ToString();
                page_Label.HorizontalAlignment = HorizontalAlignment.Center;

                StackPanel page_Panel = new StackPanel();
                page_Panel.Orientation = Orientation.Vertical;
                page_Panel.Children.Add(page_Image);
                page_Panel.Children.Add(page_Label);

                Button page_Button = new Button();
                page_Button.Content = page_Panel;
                page_Button.Name = count.ToString();
                page_Button.Click += Page_Button_Click;
                page_Button.HorizontalAlignment = HorizontalAlignment.Center;

                Windows.UI.Color ba_color = Windows.UI.Color.FromArgb(100, 219, 219, 219);
                page_Button.Background = new SolidColorBrush(ba_color);

                preview_unitHeight = page_Image.Height + 28;
                
                preview_grid.Children.Add(page_Button);
                Grid.SetColumn(page_Button, 0);
                Grid.SetRow(page_Button, count);
            }
        }

        private int Preview_LoadPage(int iPageIndex)
        {// To load and parse page.
            
            int result = -1;
            if (m_PDFDoc.pointer == 0)
            {
                return result;
            }
            if(!preview_visiblePage.ContainsKey(iPageIndex))
            {
                if(visiblePage.ContainsKey(iPageIndex))
                {
                    preview_visiblePage.Add(iPageIndex, visiblePage[iPageIndex].hPage);
                    result = 0;
                }
                else
                {
                    result = m_SDKDocument.LoadPageSync(iPageIndex);
                    if (0 == result)
                    {
                        m_PDFPage.pointer = m_SDKDocument.m_hPage.pointer;
                        PageHandle pageptr = new PageHandle();
                        pageptr.pointer = m_SDKDocument.m_hPage.pointer;
                        preview_visiblePage.Add(iPageIndex, pageptr);
                    }
                }
                
            }
            else
            {
                result = 0;
            }
            return result;
            
        }

        private void Preview_unLoadPage(int iPageIndex)
        {
            if(preview_visiblePage.ContainsKey(iPageIndex))
            {
                if(!visiblePage.ContainsKey(iPageIndex))
                {
                    m_PDFFunction.My_Page_Clear(preview_visiblePage[iPageIndex]);
                }
                
                preview_visiblePage.Remove(iPageIndex);
                Button tmp_b = (Button)(preview_grid.Children.ElementAt(iPageIndex));
                StackPanel tmp_s = (StackPanel)tmp_b.Content;
                Image img = (Image)tmp_s.Children.First();
                img.Source = null;
            } 
        }

        async void  Preview_ShowPage(int startIndex = -1, int endIndex = 0)
        {
            if (startIndex == -1)
            {
                startIndex = preview_firstVisibleIndex;
                endIndex = preview_nextInvisibleIndex;
            }
            for (int count = startIndex; count < endIndex; count++)
            {
                Button tmp_b = (Button)(preview_grid.Children.ElementAt(count));
                StackPanel tmp_s = (StackPanel)tmp_b.Content;
                Image img = (Image)tmp_s.Children.First();

                PixelSource bitmap = new PixelSource();
                bitmap.Width = (int)m_iRenderAreaSizeX / preview_scaleDown;
                bitmap.Height = (int)m_iRenderAreaSizeY / preview_scaleDown;
                if (!preview_visiblePage.ContainsKey(count))
                {
                    continue;
                }
                Windows.Storage.Streams.IRandomAccessStreamWithContentType stream = await m_SDKDocument.RenderPageAsync(bitmap, 0, 0,  bitmap.Width, bitmap.Height, 0, preview_visiblePage[count]);
                if (stream == null)
                {
                    return;
                }
                Windows.UI.Xaml.Media.Imaging.BitmapImage bmpImage = new Windows.UI.Xaml.Media.Imaging.BitmapImage();
                bmpImage.SetSource(stream);
                img.Source = bmpImage;
            }
        }

        private void Page_Button_Click(object sender, RoutedEventArgs e)
        {
            Button clicked = (Button)sender;
            int currentPage = Int32.Parse(clicked.Name);
            Jump_Page(currentPage);
        }

        private void Preview_ViewChanged(object sender, ScrollViewerViewChangedEventArgs e)
        {
            Preview_UpdareView();
        }

        private void Preview_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            Preview_UpdareView();
        }

        private void Preview_UpdareView()
        {
            if (m_PDFDoc.pointer == 0)
            {
                return;
            }
            if (m_iRenderAreaSizeY <= 0 || m_iRenderAreaSizeY <= 0)
            {
                return;
            }
            int firstVisibleIndexTemp = Math.Max((int)Math.Floor(preview_scroll.VerticalOffset / preview_unitHeight) - 1, 0);
            int nextInvisibleIndexTemp = Math.Min((int)Math.Floor((preview_scroll.VerticalOffset + preview_scroll.ActualHeight) / preview_unitHeight) + 2, m_iPageCount);
            int pageNumTemp1;
            if (firstVisibleIndexTemp != preview_firstVisibleIndex || nextInvisibleIndexTemp != preview_nextInvisibleIndex)
            {
                if (firstVisibleIndexTemp <= preview_firstVisibleIndex)
                {
                    pageNumTemp1 = Math.Min(nextInvisibleIndexTemp, preview_firstVisibleIndex);
                    if (firstVisibleIndexTemp < pageNumTemp1)
                    {
                        for (int count = firstVisibleIndexTemp; count < pageNumTemp1; count++)
                        {
                            Preview_LoadPage(count);
                        }
                        Preview_ShowPage(firstVisibleIndexTemp, pageNumTemp1);
                    }
                    if (preview_nextInvisibleIndex < nextInvisibleIndexTemp)
                    {
                        for (int count = preview_nextInvisibleIndex; count < nextInvisibleIndexTemp; count++)
                        {
                            Preview_LoadPage(count);
                        }
                        Preview_ShowPage(preview_nextInvisibleIndex, nextInvisibleIndexTemp);
                    }
                    pageNumTemp1 = Math.Max(nextInvisibleIndexTemp, preview_firstVisibleIndex);
                    for (int count = pageNumTemp1; count < preview_nextInvisibleIndex; count++)
                    {
                        Preview_unLoadPage(count);
                    }
                }
                else
                {
                    pageNumTemp1 = Math.Min(preview_nextInvisibleIndex, firstVisibleIndexTemp);
                    for (int count = preview_firstVisibleIndex; count < pageNumTemp1; count++)
                    {
                        Preview_unLoadPage(count);
                    }
                    for (int count = nextInvisibleIndexTemp; count < preview_nextInvisibleIndex; count++)
                    {
                        Preview_unLoadPage(count);
                    }
                    pageNumTemp1 = Math.Max(preview_nextInvisibleIndex, firstVisibleIndexTemp);
                    if (pageNumTemp1 < nextInvisibleIndexTemp)
                    {
                        for (int count = pageNumTemp1; count < nextInvisibleIndexTemp; count++)
                        {
                            Preview_LoadPage(count);
                        }
                        Preview_ShowPage(pageNumTemp1, nextInvisibleIndexTemp);
                    }
                }
                preview_firstVisibleIndex = firstVisibleIndexTemp;
                preview_nextInvisibleIndex = nextInvisibleIndexTemp;
            }
        }

        private void Click_PreviewPage(object sender, RoutedEventArgs e)
        {
            if (m_PDFDoc.pointer == 0)
            {
                return;
            }          

            preview_grid.Height = m_iPageCount * preview_unitHeight;
            preview_scroll.Height = scroll_pdf.ActualHeight;
            preview_popup.VerticalOffset = 0;
            preview_popup.HorizontalOffset = 0;

            preview_popup.IsOpen = true;

            UpdateLayout();

            for(int i = preview_firstVisibleIndex; i < firstVisibleIndex - (int)(preview_scroll.ActualHeight / preview_unitHeight) / 2; i++)
            {
                Preview_unLoadPage(i);
            }

            preview_firstVisibleIndex = Math.Max(firstVisibleIndex - (int)(preview_scroll.ActualHeight/preview_unitHeight)/2 , 0);
            preview_nextInvisibleIndex = Math.Min(preview_firstVisibleIndex + (int)(preview_scroll.ActualHeight / preview_unitHeight) + 2, m_iPageCount);

            for (int i = preview_firstVisibleIndex; i < preview_nextInvisibleIndex; i++)
            {
                int result = Preview_LoadPage(i);
                if (result != 0)
                    return;
            }
            Preview_ShowPage();

            preview_scroll.ScrollToVerticalOffset(preview_firstVisibleIndex * preview_unitHeight);
        }

        public void Searchword_Click(object sender, RoutedEventArgs e)
        {

        }

        private void Previous_Click(object sender, RoutedEventArgs e)
        {

        }

        private void Next_Click(object sender, RoutedEventArgs e)
        {

        }

        private async void Newpdf_Click(object sender, RoutedEventArgs e)
        {
            Windows.Storage.Pickers.FileOpenPicker openPicker = new Windows.Storage.Pickers.FileOpenPicker();
            openPicker.ViewMode = Windows.Storage.Pickers.PickerViewMode.Thumbnail;
            openPicker.SuggestedStartLocation = Windows.Storage.Pickers.PickerLocationId.PicturesLibrary;
            openPicker.FileTypeFilter.Add(".pdf");
            Windows.Storage.StorageFile pdf_file = await openPicker.PickSingleFileAsync();
            if (pdf_file != null)
            {
                this.Frame.Navigate(typeof(renderPage), pdf_file);
            }
        }

        private void closepdf_Click(object sender, RoutedEventArgs e)
        {

        }

        public bool flag_notebook = false;
        private void Like_Click(object sender, RoutedEventArgs e)
        {
            if (flag_notebook == false)
            {
                string a = "01word pron&1释义$$一个很长的释义$$一个很长长长长长长长长长长长长长长长长的释义&5释义E01词语 音标&4释义E01word pron&1释义$$一个很长的释义$$一个很长长长长长长长长长长长长长长长长的释义&5释义E01word pron&1释义$$一个很长的释义$$一个很长长长长长长长长长长长长长长长长的释义&5释义E01word pron&1释义$$一个很长的释义$$一个很长长长长长长长长长长长长长长长长的释义&5释义E01word pron&1释义$$一个很长的释义$$一个很长长长长长长长长长长长长长长长长的释义&5释义E01word pron&1释义$$一个很长的释义$$一个很长长长长长长长长长长长长长长长长的释义&5释义E01word pron&1释义$$一个很长的释义$$一个很长长长长长长长长长长长长长长长长的释义&5释义E01word pron&1释义$$一个很长的释义$$一个很长长长长长长长长长长长长长长长长的释义&5释义E01word pron&1释义$$一个很长的释义$$一个很长长长长长长长长长长长长长长长长的释义&5释义E01word pron&1释义$$一个很长的释义$$一个很长长长长长长长长长长长长长长长长的释义&5释义E";
                getNotebook(a);
                flag_notebook = true;
            }
            else
            {
                currentNotebook.Children.Clear();
                flag_notebook = false;
                GridLength hleng = new GridLength(0);
                currtbook.Width = hleng;
            }
        }

        private void getNotebook(string a)
        {
            GridLength hleng = new GridLength(250);
            currtbook.Width = hleng;
            char[] charArray = a.ToCharArray();
            string sub;
            string s_temp, s_full;
            string S_xinzhi;
            int i = 0, j = 0, k = 0, cnt = 0;
            int m = 0;
            ImageBrush brush = new ImageBrush();
            ImageBrush brush1 = new ImageBrush();
            brush.ImageSource = new BitmapImage(
                        new Uri("ms-appx:///Assets/0006.jpg", UriKind.RelativeOrAbsolute)
                   );
            while (j != a.Length)
            {
                s_full = "\0";
                for (i = j; charArray[i] != '&'; i++) { }
                sub = a.Substring(j + 2, i - j - 2);
                s_full = sub + '\n';

                while (charArray[j] != 'E')
                {
                    j = i + 2;
                    k = k + 1;
                    cnt = 0;
                    switch (charArray[i + 1])
                    {
                        case '1':
                            S_xinzhi = "prop. ";
                            break;
                        case '2':
                            S_xinzhi = "int. ";
                            break;
                        case '3':
                            S_xinzhi = "abbr. ";
                            break;
                        case '4':
                            S_xinzhi = "n. ";
                            break;
                        case '5':
                            S_xinzhi = "v. ";
                            break;
                        case '6':
                            S_xinzhi = "adj. ";
                            break;
                        case '7':
                            S_xinzhi = "pron. ";
                            break;
                        case '8':
                            S_xinzhi = "art. ";
                            break;
                        case '9':
                            S_xinzhi = "na. ";
                            break;
                        default:
                            S_xinzhi = "more. ";
                            break;
                    }
                    s_temp = S_xinzhi;
                    s_full = s_full + S_xinzhi;
                    for (i = j; charArray[i] != '&'; i++)
                    {
                        if (charArray[i] == '$' && charArray[i + 1] == '$')
                        {
                            sub = a.Substring(j, i - j);
                            i = i + 1;
                            j = i + 1;
                            cnt = cnt + 1;
                            if (cnt <= 4)
                            {
                                s_temp = s_temp + sub + ';';
                            }
                            s_full = s_full + sub + ';';
                            if (cnt == 38)
                                break;
                        }
                        if (charArray[i] == 'E')
                            break;
                    }
                    if (cnt == 38)
                    {

                        while (charArray[j] < '0' || charArray[j] > '9')
                        {
                            if (charArray[j] == 'E')
                                break;
                            j++;
                        }
                        i = j;
                        if (charArray[j] == 'E')
                            break;
                    }
                    else
                    {
                        sub = a.Substring(j, i - j);

                        cnt = cnt + 1;
                        if (cnt <= 4)
                            s_temp = s_temp + sub + ';';
                        s_full = s_full + sub + ';';

                        if (charArray[i] == 'E')
                            break;
                    }
                    s_full = s_full + '\n';

                }
                j = i + 1;
                Grid g = new Grid();
                Button close = new Button();
                TextBlock h = new TextBlock();

                g.Margin = new Thickness(12, 12, 12, 12);
                g.Background = brush;
                h.Width = 200;
                h.Height = 150;
                h.TextWrapping = TextWrapping.Wrap;
                h.Text = s_full;
                g.Children.Add(h);
                close.Content = "------";
                close.Name = "name" + m;
                g.Name = "name" + m;
                close.HorizontalAlignment = HorizontalAlignment.Right;
                close.VerticalAlignment = VerticalAlignment.Top;
                close.Height = 25;
                close.Click += Closeunit_Click;
                close.Background = brush1;
                g.Children.Add(close);
                currentNotebook.Children.Add(g);
                m = m + 1;
            }
        }

        private void Closeunit_Click(object sender, RoutedEventArgs e)
        {
            Button btn = (Button)sender;

            foreach (Grid gri in this.currentNotebook.Children)
            {

                if (gri.Name == btn.Name)
                {
                    gri.Visibility = Visibility.Collapsed;
                }

            }
        }

        bool flags_print=false;
        private  void camera_Click(object sender, RoutedEventArgs e)
        {
            GridLength hh;
            if (flags_print == false)
            {
                hh = new GridLength(100);
                hidechoice.Height = hh;
                this.PrintFrame.Navigate(typeof(Print));
                flags_print = true;
           }
            else
            {
                hh = new GridLength(0);
                hidechoice.Height = hh;
                flags_print = false;
            }
        }

        private void fullscreen_Click(object sender, RoutedEventArgs e)
        {
            var view = ApplicationView.GetForCurrentView();
            if (view.IsFullScreenMode)
            {
                view.ExitFullScreenMode();
                
            }
            else
            {
                if (view.TryEnterFullScreenMode())
                {

                }
                else
                {

                }
            }
        }
        ////////////////////////////////////////////////////////////////////////////////////////////////////////

    }

    class visibilityListener : DependencyObject
    {
        public Visibility MyProperty { get; set; }
    }
}
