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

namespace Bonako.View
{
    /// <summary>
    /// LogControl.xaml の相互作用ロジック
    /// </summary>
    public partial class LogControl : UserControl
    {
        /// <summary>
        /// コンストラクタ
        /// </summary>
        public LogControl()
        {
            InitializeComponent();

            CommandBindings.Add(
                new CommandBinding(
                    ApplicationCommands.Copy,
                    ExecuteCopyItem));
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
    }
}
