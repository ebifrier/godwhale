﻿using System;
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

namespace Bonako
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

            Closed += MainWindow_Closed;
            DataContext = Global.MainViewModel;

            Global.MainWindow = this;
        }

        void MainWindow_Closed(object sender, EventArgs e)
        {
            Global.MainWindow = null;
            Global.Quit();
        }
    }
}
