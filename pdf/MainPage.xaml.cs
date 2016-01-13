
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;
// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

namespace pdf
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        public static MainPage Current;
        public MainPage()
        {
            this.InitializeComponent();

        }

       

        public void NotifyUser(string strMessage, NotifyType type)
        {
            switch (type)
            {
                case NotifyType.StatusMessage:
                    StatusBorder.Background = new SolidColorBrush(Windows.UI.Colors.Green);
                    break;
                case NotifyType.ErrorMessage:
                    StatusBorder.Background = new SolidColorBrush(Windows.UI.Colors.Red);
                    break;
            }
            StatusBlock.Text = strMessage;

            // Collapse the StatusBlock if it has no text to conserve real estate.
            StatusBorder.Visibility = (StatusBlock.Text != String.Empty) ? Visibility.Visible : Visibility.Collapsed;
            if (StatusBlock.Text != String.Empty)
            {
                StatusBorder.Visibility = Visibility.Visible;
                StatusPanel.Visibility = Visibility.Visible;
            }
            else
            {
                StatusBorder.Visibility = Visibility.Collapsed;
                StatusPanel.Visibility = Visibility.Collapsed;
            }
        }


        private async void Click_BTN_RenderPDF(object sender, RoutedEventArgs e)
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

        private void splitViewToggle_Click(object sender, RoutedEventArgs e)
        {
            if (this.View.IsPaneOpen == true)
                this.View.IsPaneOpen = false;
            else
                if (this.View.IsPaneOpen == false)
                this.View.IsPaneOpen = true;
        }



        private void featureList_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {

            ListView scenarioListBox = sender as ListView;
            int s = scenarioListBox.SelectedIndex;
            switch (s)
            {
                case 0:
                    this.ScenarioFrame.Navigate(typeof(Notebook));
                    break;
                case 1:
                    this.Frame.Navigate(typeof(MainPage));
                    break;
                default:
                    break;
            }


        }
    }

    public enum NotifyType
    {
        StatusMessage,
        ErrorMessage
    };
}
