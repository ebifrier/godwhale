using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;

using Ragnarok;
using Ragnarok.Utility;

namespace ServerProxy
{
    /// <summary>
    /// 入出力用のスレッドデータを保持します。
    /// </summary>
    public sealed class ThreadData
    {
        /// <summary>
        /// 入力スレッドのインデックスを取得します。
        /// </summary>
        public int Index
        {
            get;
            private set;
        }

        /// <summary>
        /// 出力スレッドのインデックスを取得します。
        /// </summary>
        public int CoIndex
        {
            get { return (1 - Index); }
        }

        /// <summary>
        /// 識別用の名前を取得します。
        /// </summary>
        public string Name
        {
            get;
            private set;
        }

        /// <summary>
        /// 入力用のストリームを作成するデリゲートを取得します。
        /// </summary>
        public Func<ThreadData, Stream> StreamCreator
        {
            get;
            private set;
        }

        /// <summary>
        /// コンストラクタ
        /// </summary>
        public ThreadData(int index, string name,
                          Func<ThreadData, Stream> creator)
        {
            Index = index;
            Name = name;
            StreamCreator = creator;
        }
    }

    /// <summary>
    /// CSA将棋サーバーの指し手を中継するためのクラスです。
    /// </summary>
    public sealed class ServerProxy
    {
        private readonly Thread[] threads = new Thread[2];
        private readonly Stream[] streams = new Stream[2];

        /// <summary>
        /// スレッドを強制終了させるかどうかを取得または設定します。
        /// </summary>
        public bool Aborted
        {
            get;
            private set;
        }

        /// <summary>
        /// 入出力スレッドを取得します。
        /// </summary>
        public Thread[] Threads
        {
            get { return this.threads; }
        }

        /// <summary>
        /// サーバー間の中継処理を開始します。
        /// </summary>
        public void Start(string dstName, Func<ThreadData, Stream> dstCreator,
                          string srcName, Func<ThreadData, Stream> srcCreator)
        {
            var datas = new ThreadData[]
            {
                new ThreadData(0, dstName, dstCreator),
                new ThreadData(1, srcName, srcCreator),
            };

            for (var i = 0; i < this.threads.Count(); ++i)
            {
                var th = new Thread(ProxyThread)
                {
                    IsBackground = true,
                    Name = "ProxyThread" + i.ToString(),
                };

                th.Start(datas[i]);
                this.threads[i] = th;
            }
        }

        /// <summary>
        /// すべての処理を強制的に終了させます。
        /// </summary>
        public void Abort()
        {
            Aborted = true;
            CloseStreams();

            foreach (var th in Threads)
            {
                th.Join();
            }
        }

        /// <summary>
        /// サーバーへの接続、受信、送信などの中継処理を行います。
        /// </summary>
        private void ProxyThread(object state)
        {
            var data = (ThreadData)state;
            var timer = new Stopwatch();
            BinarySplitReader reader = null;

            this.streams[data.Index] = CreateStream(data);
            timer.Start();

            while (!Aborted)
            {
                if (reader == null)
                {
                    reader = new BinarySplitReader(2048);
                }

                var stream = this.streams[data.Index];
                if (stream == null || !stream.CanRead)
                {
                    if (Aborted)
                    {
                        break;
                    }
                    else if (timer.Elapsed < TimeSpan.FromSeconds(5))
                    {
                        continue;
                    }

                    this.streams[data.Index] = CreateStream(data);
                    timer.Restart();
                    continue;
                }

                var bytes = ReadBytes(stream, reader);
                if (bytes == null)
                {
                    Log.Info("{0}: disconnected", data.Name);

                    // 戻り値がnullの場合はストリームを閉じます。
                    CloseStreams();
                    reader = null;
                    continue;
                }

                if (!bytes.Any())
                {
                    continue;
                }

                // コンソールに出力
                WriteLog(data, bytes);

                // 書き込み先ソケットに出力します。
                WriteBytes(this.streams[data.CoIndex], bytes);
            }
        }

        private void CloseStreams()
        {
            for (var i = 0; i < this.streams.Count(); ++i)
            {
                if (this.streams[i] != null)
                {
                    this.streams[i].Close();
                    this.streams[i] = null;
                }
            }
        }

        /// <summary>
        /// コンソールに出力します。
        /// </summary>
        private void WriteLog(ThreadData data, byte[] bytes)
        {
            try
            {
                var line = Encoding.UTF8.GetString(bytes);
                line = line.TrimEnd('\n', '\r');

                Console.WriteLine("{0}> {1}", data.Name, line);
            }
            catch
            {
                // スルー
            }
        }

        /// <summary>
        /// 入出力ストリームの作成を行います。
        /// </summary>
        private Stream CreateStream(ThreadData data)
        {
            try
            {
                return data.StreamCreator(data);
            }
            catch (Exception ex)
            {
                Util.ThrowIfFatal(ex);

                Log.ErrorException(ex,
                    "ストリームの作成に失敗しました。");
            }

            return null;
        }

        /// <summary>
        /// ストリームから同期読み取りを行います。
        /// </summary>
        /// <remarks>
        /// エラー等の場合はnullを返します。
        /// </remarks>
        private byte[] ReadBytes(Stream stream, BinarySplitReader reader)
        {
            try
            {
                // すでに読み込まれたデータがあるならそれを返します。
                var bytes = reader.ReadUntil((byte)'\n');
                if (bytes != null)
                {
                    return bytes;
                }

                var buffer = new byte[256];
                var size = stream.Read(buffer, 0, buffer.Length);
                if (size == 0)
                {
                    // ソケットが閉じられたときはnullを返します。
                    return null;
                }

                reader.Write(buffer, 0, size);
                return (reader.ReadUntil((byte)'\n') ?? new byte[0]);
            }
            catch (Exception ex)
            {
                Util.ThrowIfFatal(ex);

                Log.ErrorException(ex,
                    "ソケットの受信処理に失敗しました。");
                return null;
            }
        }

        /// <summary>
        /// ソケットへの同期書き込みを行います。
        /// </summary>
        private void WriteBytes(Stream stream, byte[] buffer)
        {
            try
            {
                if (stream == null || !stream.CanWrite)
                {
                    return;
                }

                stream.Write(buffer, 0, buffer.Length);
                //stream.Flush();
            }
            catch (Exception ex)
            {
                Util.ThrowIfFatal(ex);

                Log.ErrorException(ex,
                    "ソケットへの書き込みに失敗しました。");
            }
        }
    }
}
