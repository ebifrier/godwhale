using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

using Ragnarok;
using Ragnarok.Presentation;

namespace Bonako.DFPN
{
    /// <summary>
    /// MainWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class MainWindow : Window
    {
        /// <summary>
        /// コンストラクタ
        /// </summary>
        public MainWindow()
        {
            InitializeComponent();
            CommandBindings.Add(
                new CommandBinding(
                    ApplicationCommands.Copy,
                    ExecuteCopyItem));

            Closed += MainWindow_Closed;
            DataContext = Global.MainViewModel;
        }

        static void ExecuteCopyItem(object sender, ExecutedRoutedEventArgs e)
        {
            var source = e.OriginalSource as ContentControl;
            if (source == null)
            {
                return;
            }

            Clipboard.SetText(source.Content as string);
        }

        void MainWindow_Closed(object sender, EventArgs e)
        {
            Global.Quit();
        }
    }
}
