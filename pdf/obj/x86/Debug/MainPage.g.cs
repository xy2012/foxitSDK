﻿#pragma checksum "C:\Users\v-xiafe\Desktop\foxitSDK1.4\foxitSDK\pdf\MainPage.xaml" "{406ea660-64cf-4c82-b6f0-42d48172a799}" "046D68B9FBE3222F745FE6929CF45457"
//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
// </auto-generated>
//------------------------------------------------------------------------------

namespace pdf
{
    partial class MainPage : 
        global::Windows.UI.Xaml.Controls.Page, 
        global::Windows.UI.Xaml.Markup.IComponentConnector,
        global::Windows.UI.Xaml.Markup.IComponentConnector2
    {
        /// <summary>
        /// Connect()
        /// </summary>
        [global::System.CodeDom.Compiler.GeneratedCodeAttribute("Microsoft.Windows.UI.Xaml.Build.Tasks"," 14.0.0.0")]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        public void Connect(int connectionId, object target)
        {
            switch(connectionId)
            {
            case 1:
                {
                    this.View = (global::Windows.UI.Xaml.Controls.SplitView)(target);
                }
                break;
            case 2:
                {
                    this.featureList = (global::Windows.UI.Xaml.Controls.ListView)(target);
                    #line 41 "..\..\..\MainPage.xaml"
                    ((global::Windows.UI.Xaml.Controls.ListView)this.featureList).SelectionChanged += this.featureList_SelectionChanged;
                    #line default
                }
                break;
            case 3:
                {
                    this.splitViewToggle = (global::Windows.UI.Xaml.Controls.Button)(target);
                    #line 25 "..\..\..\MainPage.xaml"
                    ((global::Windows.UI.Xaml.Controls.Button)this.splitViewToggle).Click += this.splitViewToggle_Click;
                    #line default
                }
                break;
            case 4:
                {
                    this.render = (global::Windows.UI.Xaml.Controls.Button)(target);
                    #line 30 "..\..\..\MainPage.xaml"
                    ((global::Windows.UI.Xaml.Controls.Button)this.render).Click += this.Click_BTN_RenderPDF;
                    #line default
                }
                break;
            case 5:
                {
                    this.ScenarioFrame = (global::Windows.UI.Xaml.Controls.Frame)(target);
                }
                break;
            case 6:
                {
                    this.StatusPanel = (global::Windows.UI.Xaml.Controls.StackPanel)(target);
                }
                break;
            case 7:
                {
                    this.StatusLabel = (global::Windows.UI.Xaml.Controls.TextBlock)(target);
                }
                break;
            case 8:
                {
                    this.StatusBorder = (global::Windows.UI.Xaml.Controls.Border)(target);
                }
                break;
            case 9:
                {
                    this.StatusBlock = (global::Windows.UI.Xaml.Controls.TextBlock)(target);
                }
                break;
            case 10:
                {
                    this.StatusBlock1 = (global::Windows.UI.Xaml.Controls.TextBlock)(target);
                }
                break;
            default:
                break;
            }
            this._contentLoaded = true;
        }

        [global::System.CodeDom.Compiler.GeneratedCodeAttribute("Microsoft.Windows.UI.Xaml.Build.Tasks"," 14.0.0.0")]
        [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
        public global::Windows.UI.Xaml.Markup.IComponentConnector GetBindingConnector(int connectionId, object target)
        {
            global::Windows.UI.Xaml.Markup.IComponentConnector returnValue = null;
            return returnValue;
        }
    }
}

