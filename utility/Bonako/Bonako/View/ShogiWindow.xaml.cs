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
using System.Windows.Shapes;
using System.Windows.Threading;

using Ragnarok;
using Ragnarok.Presentation.Utility;

namespace Bonako.View
{
    using ViewModel;

    /// <summary>
    /// ShogiWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class ShogiWindow : Window
    {
        /// <summary>
        /// エフェクト管理用オブジェクトを取得します。
        /// </summary>
        public EffectManager EffectManager
        {
            get;
            private set;
        }

        private FrameTimer timer;

        public ShogiWindow()
        {
            InitializeComponent();
            ShogiControl.InitializeBindings(this);

            EffectManager = new EffectManager()
            {
                Background = this.visualBackground,
            };

            ShogiControl.EffectManager = EffectManager;
            DataContext = Global.ShogiModel;

            Global.ShogiWindow = this;
            Closed += (_, __) => Global.ShogiWindow = null;

            EffectManager.ChangeMoveCount(1);

            this.timer = new FrameTimer(
                30,
                timer_EnterFrame,
                Dispatcher);
            this.timer.Start();
        }

        void timer_EnterFrame(object sender, FrameEventArgs e)
        {
            // フレーム時間が長すぎると、バグるエフェクトがあるため、
            // 時間を適度な短さに調整しています。
            var MaxFrameTime = TimeSpan.FromMilliseconds(1000.0 / 20);
            var elapsed = MathEx.Min(e.ElapsedTime, MaxFrameTime);

            if (ShogiControl != null)
            {
                ShogiControl.Render(elapsed);
            }

            if (this.visualBackground != null)
            {
                this.visualBackground.Render(elapsed);
            }
        }
    }
}
