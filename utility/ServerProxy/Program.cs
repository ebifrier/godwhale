using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Threading;

using Ragnarok;

namespace ServerProxy
{
    class Program
    {
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

                return new NetworkStream(socket);
            }
            catch (Exception ex)
            {
                Util.ThrowIfFatal(ex);

                Log.ErrorException(ex,
                    "'{0}:{1}'への接続に失敗しました。",
                    address, port);
                Thread.Sleep(10 * 1000);
            }

            return null;
        }

        static void Main(string[] args)
        {
            var proxy = new ServerProxy();

            // 将棋サーバーのアドレスとポートです。
            string ShogiServerAddress = "192.168.20.1";
            int ShogiServerPort = 4081;

            // 大合神クジラちゃんのアドレスとポートです。
            string GodWhaleServerAddress = "54.178.196.154";
            int GodWhaleServerPort = 4090;

            proxy.Start(
                "CSA", _ => Connect(_, ShogiServerAddress, ShogiServerPort),
                "god", _ => Connect(_, GodWhaleServerAddress, GodWhaleServerPort));

            foreach (var th in proxy.Threads)
            {
                th.Join();
            }
        }
    }
}
