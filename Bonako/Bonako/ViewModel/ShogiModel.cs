﻿using System;
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
        private DateTime prevUpdateTime = DateTime.Now;
        private DateTime lastPlayedTime = DateTime.Now;
        private List<string> parsedCommandList = new List<string>();

        /// <summary>
        /// 現局面を取得します。
        /// </summary>
        public Board CurrentBoard
        {
            get { return GetValue<Board>("CurrentBoard"); }
            private set { SetValue("CurrentBoard", value); }
        }

        /// <summary>
        /// 表示局面を取得または設定します。
        /// </summary>
        public Board Board
        {
            get { return GetValue<Board>("Board"); }
            set { SetValue("Board", value); }
        }

        /// <summary>
        /// 自分の手番を取得または設定します。
        /// </summary>
        public BWType MyTurn
        {
            get { return GetValue<BWType>("MyTurn"); }
            set { SetValue("MyTurn", value); }
        }

        /// <summary>
        /// 今の手番を取得または設定します。
        /// </summary>
        public BWType CurrentTurn
        {
            get { return GetValue<BWType>("CurrentTurn"); }
            set { SetValue("CurrentTurn", value); }
        }

        /// <summary>
        /// 現在の評価値を取得または設定します。
        /// </summary>
        public double CurrentEvaluationValue
        {
            get { return GetValue<double>("CurrentEvaluationValue"); }
            set { SetValue("CurrentEvaluationValue", value); }
        }

        /// <summary>
        /// 評価値を取得または設定します。
        /// </summary>
        public double EvaluationValue
        {
            get { return GetValue<double>("EvaluationValue"); }
            set { SetValue("EvaluationValue", value); }
        }

        /// <summary>
        /// 先手の対局者名を取得または設定します。
        /// </summary>
        public string BlackPlayerName
        {
            get { return GetValue<string>("BlackPlayerName"); }
            set { SetValue("BlackPlayerName", value); }
        }

        /// <summary>
        /// 後手の対局者名を取得または設定します。
        /// </summary>
        public string WhitePlayerName
        {
            get { return GetValue<string>("WhitePlayerName"); }
            set { SetValue("WhitePlayerName", value); }
        }

        /// <summary>
        /// 先手の一手前までの残り時間を取得または設定します。
        /// </summary>
        public TimeSpan BlackBaseLeaveTime
        {
            get { return GetValue<TimeSpan>("BlackBaseLeaveTime"); }
            set { SetValue("BlackBaseLeaveTime", value); }
        }

        /// <summary>
        /// 後手の一手前までの残り時間を取得または設定します。
        /// </summary>
        public TimeSpan WhiteBaseLeaveTime
        {
            get { return GetValue<TimeSpan>("WhiteBaseLeaveTime"); }
            set { SetValue("WhiteBaseLeaveTime", value); }
        }

        /// <summary>
        /// 先手の残り時間を取得または設定します。
        /// </summary>
        public TimeSpan BlackLeaveTime
        {
            get { return GetValue<TimeSpan>("BlackLeaveTime"); }
            private set { SetValue("BlackLeaveTime", value); }
        }

        /// <summary>
        /// 後手の残り時間を取得または設定します。
        /// </summary>
        public TimeSpan WhiteLeaveTime
        {
            get { return GetValue<TimeSpan>("WhiteLeaveTime"); }
            private set { SetValue("WhiteLeaveTime", value); }
        }

        /// <summary>
        /// 読み筋リストを取得します。
        /// </summary>
        public NotifyCollection<VariationInfo> VariationList
        {
            get { return GetValue<NotifyCollection<VariationInfo>>("VariationList"); }
            private set { SetValue("VariationList", value); }
        }

        /// <summary>
        /// 現局面と表示局面を設定します。
        /// </summary>
        /// <remarks>
        /// 現局面にはインスタンスをそのまま設定します。
        /// </remarks>
        public void InitBoard(Board board, bool clearVariation)
        {
            if (board == null)
            {
                return;
            }

            CurrentBoard = board;
            Board = board.Clone();
            CurrentTurn = board.Turn;
            if (clearVariation)
            {
                ClearVariationList();
            }

            // すぐに自動再生が開始するのを防ぎます。
            this.lastPlayedTime = DateTime.Now;

            // 背景イメージの設定
            var manager = Global.EffectManager;
            if (manager != null)
            {
                manager.ChangeMoveCount(board.MoveCount);
            }
        }

        /// <summary>
        /// 解析済みのコマンドリストをクリアします。
        /// </summary>
        public void ClearParsedCommand()
        {
            this.parsedCommandList.Clear();
        }

        /// <summary>
        /// 現局面の指し手を進めます。
        /// </summary>
        public void DoMove(CsaMove csaMove, int seconds)
        {
            csaMove.Side = CurrentBoard.Turn;

            var bmove = CurrentBoard.ConvertCsaMove(csaMove);
            if (bmove == null || !bmove.Validate())
            {
                Log.Error("{0}手目 {1}を変換できませんでした。",
                    CurrentBoard.MoveCount,
                    csaMove.ToPersonalString());
                return;
            }

            if (!CurrentBoard.DoMove(bmove))
            {
                Log.Error("{0}手目 {1}を指せませんでした。",
                    CurrentBoard.MoveCount,
                    csaMove.ToPersonalString());
                return;
            }

            // 実際に指した手と一致する変化は残します。
            WPFUtil.UIProcess(() =>
            {
                var list = VariationList
                    .Where(_ => _.MoveList.Count() >= 2)
                    .Where(_ => csaMove.Equals(_.MoveList[0]))
                    .Select(_ =>
                        new VariationInfo
                        {
                            IsShowed = false,
                            MoveList = _.MoveList.Skip(1).ToList(),
                            Value = _.Value,
                        });

                VariationList.Clear();
                list.ForEach(_ => VariationList.Add(_));
            });

            // 手番側の残り時間を減らしたのち、手番を入れ替えます。
            DecBaseLeaveTime(CurrentTurn, seconds);
            InitBoard(CurrentBoard, false);
        }

        /// <summary>
        /// 指定の手番側の残り時間を減らします。
        /// </summary>
        public void DecBaseLeaveTime(BWType turn, int seconds)
        {
            if (turn == BWType.Black)
            {
                BlackBaseLeaveTime = MathEx.Max(
                    BlackBaseLeaveTime - TimeSpan.FromSeconds(seconds),
                    TimeSpan.Zero);
            }
            else
            {
                WhiteBaseLeaveTime = MathEx.Max(
                    WhiteBaseLeaveTime - TimeSpan.FromSeconds(seconds),
                    TimeSpan.Zero);
            }
        }

        /// <summary>
        /// 自分の残り時間を減らします。
        /// </summary>
        public void DecMyLeaveTime(int seconds)
        {
            DecBaseLeaveTime(MyTurn, seconds);
        }

        private IEnumerable<BoardMove> GetVariationMoveList(VariationInfo variation)
        {
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

            // ７手以下の場合は、再生しません。
            return (bmoveList.Count < 7 ? null : bmoveList);
        }

        /// <summary>
        /// 次の変化を自動再生します。
        /// </summary>
        public void PlayNextVariation()
        {
            WPFUtil.UIProcess(() =>
            {
                while (true)
                {
                    // まだ未表示の変化を探します。
                    var variation = VariationList
                        .FirstOrDefault(_ => !_.IsShowed);
                    if (variation == null)
                    {
                        break;
                    }

                    // 手数が少ないなどの場合は、変化を再生しません。
                    var bmoveList = GetVariationMoveList(variation);
                    if (bmoveList == null)
                    {
                        variation.IsShowed = true;
                        continue;
                    }

                    PlayVariation(variation, bmoveList);
                    break;
                }
            });
        }

        /// <summary>
        /// 可能なら変化の自動再生を開始します。
        /// </summary>
        /// <remarks>
        /// GUIスレッドで呼んでください。
        /// </remarks>
        private void PlayVariation(VariationInfo variation,
                                   IEnumerable<BoardMove> bmoveList)
        {
            var shogi = Global.ShogiControl;
            if (shogi == null || shogi.AutoPlayState == AutoPlayState.Playing)
            {
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

            // 評価値を表示している変化に合わせます。
            EvaluationValue = variation.Value;

            // 再生済みフラグを立ててしまいます。
            variation.IsShowed = true;

            shogi.StartAutoPlay(autoPlay);
        }

        void AutoPlay_Stopped(object sender, EventArgs e)
        {
            Board = CurrentBoard.Clone();
            EvaluationValue = CurrentEvaluationValue;

            this.lastPlayedTime = DateTime.Now;
        }

        /// <summary>
        /// 新しい変化を追加します。
        /// </summary>
        public void AddVariation(VariationInfo variation)
        {
            if (variation == null || variation.MoveList == null)
            {
                return;
            }

            /*if (variation.MoveList.Count() < 9)
            {
                return;
            }*/

            WPFUtil.UIProcess(() =>
            {
                // 先後の符号は省略されていることがあります。
                /*var color = CurrentBoard.Turn;
                foreach (var move in variation.MoveList)
                {
                    move.Side = color;
                    color = color.Toggle();
                }*/

                // 同じ変化があれば登録しません。
                var result = VariationList.FirstOrDefault(
                    _ => variation.MoveList.SequenceEqual(_.MoveList));
                if (result != null)
                {
                    result.Value = Math.Max(result.Value, variation.Value);
                    return;
                }

                // 変化は評価値順に並べます。
                // ObservableCollectionはソートがやりにくいので、
                // 挿入ソートを使います。
                var inserted = false;
                for (var i = 0; i < VariationList.Count(); ++i)
                {
                    if (variation.Value > VariationList[i].Value)
                    {
                        VariationList.Insert(i, variation);
                        inserted = true;
                        break;
                    }
                }

                if (!inserted)
                {
                    VariationList.Add(variation);
                }
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
        /// 残り時間を更新します。
        /// </summary>
        private void UpdateLeaveTime(TimeSpan elapsedTime)
        {
            if (CurrentTurn == BWType.Black)
            {
                BlackLeaveTime = MathEx.Max(
                    BlackLeaveTime - elapsedTime,
                    TimeSpan.Zero);
            }
            else
            {
                WhiteLeaveTime = MathEx.Max(
                    WhiteLeaveTime - elapsedTime,
                    TimeSpan.Zero);
            }
        }

        /// <summary>
        /// 定期的に呼ばれ、自動再生の開始を行います。
        /// </summary>
        private void OnTimer()
        {
            var now = DateTime.Now;
            var elapsedTime = now - this.prevUpdateTime;
            this.prevUpdateTime = now;

            // 時間の更新は必ず行います。
            UpdateLeaveTime(elapsedTime);

            // 変化の表示は一定時間経っていたら行います。
            var nextTime = this.lastPlayedTime + AutoPlayRestTime;
            if (now > nextTime)
            {
                PlayNextVariation();
            }
        }

        /// <summary>
        /// コンストラクタ
        /// </summary>
        public ShogiModel()
        {
            CurrentBoard = new Board();
            Board = new Board();
            VariationList = new NotifyCollection<VariationInfo>();

            AddPropertyChangedHandler("BlackBaseLeaveTime",
                (_, __) => BlackLeaveTime = BlackBaseLeaveTime);
            AddPropertyChangedHandler("WhiteBaseLeaveTime",
                (_, __) => WhiteLeaveTime = WhiteBaseLeaveTime);

            MyTurn = BWType.Black;
            CurrentTurn = BWType.Black;
            BlackBaseLeaveTime = TimeSpan.Zero;
            WhiteBaseLeaveTime = TimeSpan.Zero;

            this.timer = new DispatcherTimer(
                TimeSpan.FromSeconds(0.1),
                DispatcherPriority.Normal,
                (_, __) => OnTimer(),
                WPFUtil.UIDispatcher);
            this.timer.Start();
        }

        void bonanza_ReceivedCommand(object sender, BonanzaReceivedCommandEventArgs e)
        {
            try
            {
                // Bonanzaのコマンドは同じものが２回くることがあるため、
                // 重複を避けています。
                if (this.parsedCommandList.IndexOf(e.Command) >= 0)
                {
                    return;
                }
                if (this.parsedCommandList.Count() >= 3)
                {
                    this.parsedCommandList.RemoveAt(0);
                }
                this.parsedCommandList.Add(e.Command);

                Log.Debug("< {0}", e.Command);
                BonanzaCommandParser.Parse(e.Command);
            }
            catch (Exception ex)
            {
                Util.ThrowIfFatal(ex);

                Log.ErrorException(ex,
                    "{0}: コマンドの解析に失敗しました。",
                    e.Command);
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
