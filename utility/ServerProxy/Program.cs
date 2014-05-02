using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using System.Runtime.InteropServices;

using Ragnarok;

namespace ServerProxy
{
    /// <summary>
    /// ハンドラ・ルーチンに渡される定数の定義
    /// </summary>
    public enum CtrlTypes
    {
        CTRL_C_EVENT = 0,
        CTRL_BREAK_EVENT = 1,
        CTRL_CLOSE_EVENT = 2,
        CTRL_LOGOFF_EVENT = 5,
        CTRL_SHUTDOWN_EVENT = 6
    }

    class Program
    {
        delegate bool HandlerRoutine(CtrlTypes CtrlType);

        [DllImport("kernel32")]
        static extern bool SetConsoleCtrlHandler(HandlerRoutine Handler, bool Add);

        static ServerProxy proxy = new ServerProxy();

        static void Main(string[] args)
        {
            SetConsoleCtrlHandler(OnExit, true);

            // 将棋サーバーのアドレスとポートです。
            //string ShogiServerAddress = "192.168.20.1";
            //int ShogiServerPort = 4081;
            string ShogiServerAddress = "133.242.205.114";
            int ShogiServerPort = 4081;

            // 大合神クジラちゃんのアドレスとポートです。
            //string GodWhaleServerAddress = "54.178.196.154";
            //int GodWhaleServerPort = 4090;
            string GodWhaleServerAddress = "localhost";
            int GodWhaleServerPort = 4081;

            proxy.Start(
                "CSA", _ => Connect(_, ShogiServerAddress, ShogiServerPort),
                "god", _ => Connect(_, GodWhaleServerAddress, GodWhaleServerPort));

            foreach (var th in proxy.Threads)
            {
                th.Join();
            }
        }

        /// <summary>
        /// ソケットストリームを作成します。
        /// </summary>
        private static Stream Connect(ThreadData data, string address, int port)
        {
            try
            {
                var socket = new Socket(
                    AddressFamily.InterNetwork,
                    SocketType.Stream,
                    ProtocolType.Tcp);

                socket.Connect(address, port);

                Log.Info("{0}: connected", data.Name);

                return new NetworkStream(socket, true);
            }
            catch (Exception ex)
            {
                Util.ThrowIfFatal(ex);

                Log.ErrorException(ex,
                    "'{0}:{1}'への接続に失敗しました。",
                    address, port);
            }

            return null;
        }

        /// <summary>
        /// 終了時に呼ばれるハンドラ
        /// </summary>
        private static bool OnExit(CtrlTypes ctrlType)
        {
            Console.WriteLine("強制終了：" + ctrlType);

            proxy.Abort();
            return false;
        }
    }
}
