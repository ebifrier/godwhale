using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;

using Ragnarok;
using Ragnarok.Utility;

namespace ServerProxy
{
    internal sealed class ThreadData
    {
        public int Index
        {
            get;
            set;
        }

        public int CoIndex
        {
            get { return (1 - Index); }
        }

        public Func<Stream> StreamCreator
        {
            get;
            set;
        }

        /// <summary>
        /// コンストラクタ
        /// </summary>
        public ThreadData(int index, Func<Stream> creator)
        {
            Index = index;
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
        /// 入出力スレッドを取得します。
        /// </summary>
        public Thread[] Threads
        {
            get { return this.threads; }
        }

        /// <summary>
        /// サーバー間の中継処理を開始します。
        /// </summary>
        public void Start(Func<Stream> dstCreator,
                          Func<Stream> srcCreator)
        {
            var datas = new ThreadData[]
            {
                new ThreadData(0, dstCreator),
                new ThreadData(1, srcCreator),
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
        /// サーバーへの接続、受信、送信などの中継処理を行います。
        /// </summary>
        private void ProxyThread(object state)
        {
            var data = (ThreadData)state;
            BinarySplitReader reader = null;

            while (true)
            {
                if (reader == null)
                {
                    reader = new BinarySplitReader(2048);
                }

                var stream = this.streams[data.Index];
                if (stream == null || !stream.CanRead)
                {
                    this.streams[data.Index] = CreateStream(data);
                    continue;
                }

                var bytes = ReadBytes(stream, reader);
                if (bytes == null)
                {
                    // 戻り値がnullの場合はストリームを閉じます。
                    stream.Close();

                    this.streams[data.Index] = null;
                    reader = null;
                    continue;
                }

                if (!bytes.Any())
                {
                    continue;
                }

                // コンソールに出力
                try
                {
                    var line = Encoding.UTF8.GetString(bytes);
                    line.TrimEnd('\n');

                    Console.Write(line);
                }
                catch
                {
                    // スルー
                }

                // 書き込み先ソケットに出力します。
                WriteBytes(this.streams[data.CoIndex], bytes);
            }
        }

        /// <summary>
        /// 入出力ストリームの作成を行います。
        /// </summary>
        private Stream CreateStream(ThreadData data)
        {
            try
            {
                return data.StreamCreator();
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
