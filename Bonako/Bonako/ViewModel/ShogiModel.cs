using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Threading;

using Ragnarok;
using Ragnarok.Shogi;
using Ragnarok.Shogi.Bonanza;
using Ragnarok.Shogi.Csa;
using Ragnarok.ObjectModel;
using Ragnarok.Presentation;
using Ragnarok.Presentation.Shogi;

namespace Bonako.ViewModel
{
    /// <summary>
    /// 将棋曲面用のモデルオブジェクトです。
    /// </summary>
    public sealed class ShogiModel : NotifyObject
    {
        /// <summary>
        /// 自動再生と自動再生の間の最小時間間隔です。
        /// </summary>
        private static readonly TimeSpan AutoPlayRestTime =
            TimeSpan.FromSeconds(5.0);

        private DispatcherTimer timer;
        private DateTime lastPlayedTime = DateTime.Now;

        /// <summary>
        /// 現局面を取得します。
        /// </summary>
        public Board CurrentBoard
        {
            get { return GetValue<Board>("CurrentBoard"); }
            private set { SetValue("CurrentBoard", value); }
        }

        /*/// <summary>
        /// 指し手思考の起点となる局面を取得します。
        /// </summary>
        public Board PonderBoard
        {
            get { return GetValue<Board>("PonderBoard"); }
            private set { SetValue("PonderBoard", value); }
        }*/

        /// <summary>
        /// 表示局面を取得または設定します。
        /// </summary>
        public Board Board
        {
            get { return GetValue<Board>("Board"); }
            set { SetValue("Board", value); }
        }

        /*/// <summary>
        /// 先読み中の指し手一覧を取得します。
        /// </summary>
        public NotifyCollection<BoardMove> PonderMoveList
        {
            get;
            private set;
        }*/

        /// <summary>
        /// 読み筋リストを取得します。
        /// </summary>
        public NotifyCollection<VariationInfo> VariationList
        {
            get;
            private set;
        }

        /// <summary>
        /// 現局面と表示局面を設定します。
        /// </summary>
        /// <remarks>
        /// 現局面にはインスタンスをそのまま設定します。
        /// </remarks>
        public void InitBoard(Board board)
        {
            if (board == null)
            {
                return;
            }

            CurrentBoard = board;
            //PonderBoard = board.Clone();
            Board = board.Clone();
            //PonderMoveList.Clear();
            ClearVariationList();

            // すぐに自動再生が開始するのを防ぎます。
            this.lastPlayedTime = DateTime.Now;

            // 背景イメージの設定
            var manager = Global.EffectManager;
            if (manager != null)
            {
                manager.ChangeMoveCount(board.MoveCount);
            }
        }

        /*/// <summary>
        /// 先読みを一手行います。
        /// </summary>
        public bool DoPonderMove(CsaMove csaMove)
        {
            if (csaMove == null)
            {
                return false;
            }

            var bmove = PonderBoard.ConvertCsaMove(csaMove);
            if (bmove == null || !bmove.Validate())
            {
                return false;
            }

            PonderBoard.DoMove(bmove);
            PonderMoveList.Add(bmove);
            return true;
        }

        /// <summary>
        /// 先読み中の手を一手、確定した指し手として処理します。
        /// </summary>
        public void PonderHit()
        {
            if (!PonderMoveList.Any())
            {
                return;
            }

            CurrentBoard.DoMove(PonderMoveList[0]);
            PonderMoveList.RemoveAt(0);
        }
        
        /// <summary>
        /// 局面を<paramref name="nback"/>手元に戻し、さらに
        /// <paramref name="csaMove"/>で示される手を指します。
        /// </summary>
        public bool DoCsaMove(CsaMove csaMove, int nback)
        {
            if (csaMove == null)
            {
                return false;
            }

            while (nback-- > 0 && PonderBoard.CanUndo)
            {
                PonderBoard.Undo();
            }

            var bmove = PonderBoard.ConvertCsaMove(csaMove);
            if (bmove == null || !bmove.Validate())
            {
                return false;
            }

            PonderBoard.DoMove(bmove);
            InitBoard(PonderBoard.Clone());
            return true;
        }*/

        /// <summary>
        /// 次の変化を自動再生します。
        /// </summary>
        public void PlayNextVariation()
        {
            WPFUtil.UIProcess(() =>
            {
                // まだ未表示の変化を探します。
                var variation = VariationList
                    .FirstOrDefault(_ => !_.IsShowed);

                if (variation != null)
                {
                    PlayVariation(variation);
                }
            });
        }

        /// <summary>
        /// 可能なら変化の自動再生を開始します。
        /// </summary>
        /// <remarks>
        /// GUIスレッドで呼んでください。
        /// </remarks>
        private void PlayVariation(VariationInfo variation)
        {
            var shogi = Global.ShogiControl;
            if (shogi == null || shogi.AutoPlayState == AutoPlayState.Playing)
            {
                return;
            }

            // 変化は先読み後の局面で考えます。
            var board = CurrentBoard.Clone();
            var bmoveList = new List<BoardMove>();

            foreach (var csaMove in variation.MoveList)
            {
                var bmove = board.ConvertCsaMove(csaMove);
                if (bmove == null)
                {
                    break;
                }

                if (!board.DoMove(bmove))
                {
                    break;
                }

                bmoveList.Add(bmove);
            }

            // ５手以下の場合は、再生しません。
            if (bmoveList.Count < 5)
            {
                variation.IsShowed = true;
                return;
            }

            // 引数にBoardを渡すことで、Boardそのものを変えながら
            // 自動再生を行うようにします。
            Board = CurrentBoard.Clone();
            var autoPlay = new AutoPlay(Board, bmoveList)
            {
                IsChangeBackground = true,
            };
            autoPlay.Stopped += AutoPlay_Stopped;

            // 再生済みフラグを立ててしまいます。
            variation.IsShowed = true;

            shogi.StartAutoPlay(autoPlay);
        }

        void AutoPlay_Stopped(object sender, EventArgs e)
        {
            Board = CurrentBoard.Clone();

            this.lastPlayedTime = DateTime.Now;
        }

        /// <summary>
        /// 新しい変化を追加します。
        /// </summary>
        public void AddVariation(VariationInfo variation)
        {
            if (variation == null)
            {
                return;
            }

            WPFUtil.UIProcess(() =>
            {
                if (VariationList.Count > 5)
                {
                    VariationList.RemoveAt(0);
                }

                VariationList.Add(variation);

                //PlayNextVariation();
            });
        }

        /// <summary>
        /// 変化リストをクリアします。
        /// </summary>
        public void ClearVariationList()
        {
            WPFUtil.UIProcess(() =>
                VariationList.Clear());
        }

        /// <summary>
        /// 定期的に呼ばれ、自動再生の開始を行います。
        /// </summary>
        private void OnTimer()
        {
            var nextTime = this.lastPlayedTime + AutoPlayRestTime;
            if (DateTime.Now < nextTime)
            {
                return;
            }

            PlayNextVariation();
        }

        /// <summary>
        /// コンストラクタ
        /// </summary>
        public ShogiModel()
        {
            CurrentBoard = new Board();
            //PonderBoard = new Board();
            Board = new Board();
            //PonderMoveList = new NotifyCollection<BoardMove>();
            VariationList = new NotifyCollection<VariationInfo>();

            this.timer = new DispatcherTimer(
                TimeSpan.FromSeconds(0.5),
                DispatcherPriority.Normal,
                (_, __) => OnTimer(),
                WPFUtil.UIDispatcher);
            this.timer.Start();
        }

        void bonanza_ReceivedCommand(object sender, BonanzaReceivedCommandEventArgs e)
        {
            try
            {
                BonanzaCommandParser.Parse(e.Command);
            }
            catch (Exception ex)
            {
                Util.ThrowIfFatal(ex);

                return;
            }
        }

        /// <summary>
        /// ボナンザを設定します。
        /// </summary>
        public void SetBonanza(Bonanza bonanza)
        {
            bonanza.CommandReceived += bonanza_ReceivedCommand;
        }
    }
}
